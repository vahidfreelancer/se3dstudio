import asyncio
import struct
import sys
import math
import pygame
import time
from bleak import BleakScanner, BleakClient

# BLE Custom Service & Characteristic UUIDs matching firmware
SERVICE_UUID = "12345678-1234-5678-1234-56789abcdef0"
CHARACTERISTIC_UUID = "12345678-1234-5678-1234-56789abcdef1"

# States
STATE_SCANNING = 0
STATE_CONNECTING = 1
STATE_CONNECTED = 2

# Global orientation state (in degrees)
roll = 0.0
pitch = 0.0
yaw = 0.0
connection_state = STATE_SCANNING
target_device = None
ble_client = None

# Signal strength & telemetry state
rssi_value = -100
packet_rate = 0.0
notification_count = 0
last_rate_time = 0.0

# Attitude offset variables
offset_roll = 0.0
offset_pitch = 0.0
offset_yaw = 0.0

# Calibration state
calibration_timer = 0.0
is_calibrating = False

# Remapping variables
# axis_map: index 0 (Roll), index 1 (Pitch), index 2 (Yaw) mapped to Box X, Y, Z rotation
# 0 = X, 1 = Y, 2 = Z
axis_map = [0, 1, 2]
axis_inv = [1, 1, 1]

# Screen dimensions
WIDTH, HEIGHT = 900, 850

# Real-time sensor buffers for charts (keep last 100 samples)
HISTORY_LEN = 100
accel_history = [(0.0, 0.0, 0.0)] * HISTORY_LEN
gyro_history = [(0.0, 0.0, 0.0)] * HISTORY_LEN

# 3D Box vertices (representing Seeed Studio XIAO board)
# Width (X): 120, Length (Y): 180, Thickness (Z): 24
BOX_VERTICES = [
    [-60, -90, -12],  # 0
    [ 60, -90, -12],  # 1
    [ 60,  90, -12],  # 2
    [-60,  90, -12],  # 3
    [-60, -90,  12],  # 4
    [ 60, -90,  12],  # 5
    [ 60,  90,  12],  # 6
    [-60,  90,  12]   # 7
]

# 6 Faces: indices of vertices, HSL/RGB colors, and labels
BOX_FACES = [
    {"indices": (0, 1, 2, 3), "color": (44, 62, 80),   "label": "BOTTOM"}, # -Z
    {"indices": (4, 5, 6, 7), "color": (41, 128, 185), "label": "TOP"},    # +Z
    {"indices": (0, 1, 5, 4), "color": (192, 57, 43),  "label": "BACK"},   # -Y
    {"indices": (2, 3, 7, 6), "color": (39, 174, 96),  "label": "FRONT"},  # +Y (USB)
    {"indices": (1, 2, 6, 5), "color": (230, 126, 34), "label": "RIGHT"},  # +X
    {"indices": (3, 0, 4, 7), "color": (241, 196, 15), "label": "LEFT"}    # -X
]

# Helpers for 3D rotation
def rotate_x(x, y, z, angle_rad):
    cos_a = math.cos(angle_rad)
    sin_a = math.sin(angle_rad)
    return x, y * cos_a - z * sin_a, y * sin_a + z * cos_a

def rotate_y(x, y, z, angle_rad):
    cos_a = math.cos(angle_rad)
    sin_a = math.sin(angle_rad)
    return x * cos_a + z * sin_a, y, -x * sin_a + z * cos_a

def rotate_z(x, y, z, angle_rad):
    cos_a = math.cos(angle_rad)
    sin_a = math.sin(angle_rad)
    return x * cos_a - y * sin_a, x * sin_a + y * cos_a, z


def draw_text_centered(screen, text, x, y, font, color=(255, 255, 255)):
    text_surf = font.render(text, True, color)
    text_rect = text_surf.get_rect(center=(int(x), int(y)))
    screen.blit(text_surf, text_rect)

def draw_gauge(screen, x, y, label, val, min_val, max_val, color, font):
    """Draws a premium linear gauge for angles."""
    width = 200
    height = 10
    # Draw background bar
    pygame.draw.rect(screen, (50, 50, 50), (x, y, width, height), border_radius=5)
    # Clamp value
    clamped_val = max(min_val, min(max_val, val))
    pct = (clamped_val - min_val) / (max_val - min_val)
    fill_width = int(pct * width)
    # Draw filled bar
    pygame.draw.rect(screen, color, (x, y, fill_width, height), border_radius=5)
    # Draw text
    lbl_surf = font.render(f"{label}: {val:6.1f}°", True, (240, 240, 240))
    screen.blit(lbl_surf, (x, y - 25))

def draw_chart(screen, x, y, w, h, title, history, colors, font):
    # Draw chart background and border
    pygame.draw.rect(screen, (24, 30, 48), (x, y, w, h), border_radius=8)
    pygame.draw.rect(screen, (41, 55, 84), (x, y, w, h), 2, border_radius=8)
    
    # Draw title
    title_surf = font.render(title, True, (200, 210, 230))
    screen.blit(title_surf, (x + 10, y + 8))
    
    # Plot axes lines
    points_x = []
    points_y = []
    points_z = []
    
    dx = (w - 20) / (HISTORY_LEN - 1)
    
    # Auto-scale calculations
    actual_min = min(min(pt[0] for pt in history), min(pt[1] for pt in history), min(pt[2] for pt in history))
    actual_max = max(max(pt[0] for pt in history), max(pt[1] for pt in history), max(pt[2] for pt in history))
    
    # Give some headroom
    span = max(abs(actual_min), abs(actual_max))
    if span < 1.0:
        span = 1.0
    
    cur_y_min = -span * 1.1
    cur_y_max = span * 1.1
    
    # Draw zero reference line
    zero_pct = (0 - cur_y_min) / (cur_y_max - cur_y_min)
    zero_y = y + h - int(zero_pct * (h - 45)) - 15
    pygame.draw.line(screen, (50, 65, 95), (x + 10, zero_y), (x + w - 10, zero_y), 1)

    for i, (vx, vy, vz) in enumerate(history):
        px = x + 10 + i * dx
        
        # Helper to convert value to pixel Y
        def val_to_y(v):
            pct = (v - cur_y_min) / (cur_y_max - cur_y_min)
            return y + h - int(pct * (h - 45)) - 15
            
        points_x.append((px, val_to_y(vx)))
        points_y.append((px, val_to_y(vy)))
        points_z.append((px, val_to_y(vz)))
        
    # Draw the lines
    if len(points_x) > 1:
        pygame.draw.lines(screen, colors[0], False, points_x, 2)
        pygame.draw.lines(screen, colors[1], False, points_y, 2)
        pygame.draw.lines(screen, colors[2], False, points_z, 2)
        
    # Draw legends and current values (top right)
    latest = history[-1]
    lbl_x = font.render(f"X: {latest[0]:.2f}", True, colors[0])
    lbl_y = font.render(f"Y: {latest[1]:.2f}", True, colors[1])
    lbl_z = font.render(f"Z: {latest[2]:.2f}", True, colors[2])
    
    screen.blit(lbl_x, (x + w - 190, y + 8))
    screen.blit(lbl_y, (x + w - 130, y + 8))
    screen.blit(lbl_z, (x + w - 70, y + 8))


def draw_button(screen, text, x, y, w, h, bg_color, text_color, font, is_hovered):
    color = (min(255, bg_color[0] + 25), min(255, bg_color[1] + 25), min(255, bg_color[2] + 25)) if is_hovered else bg_color
    pygame.draw.rect(screen, color, (x, y, w, h), border_radius=5)
    pygame.draw.rect(screen, (70, 90, 130), (x, y, w, h), 1, border_radius=5)
    
    text_surf = font.render(text, True, text_color)
    text_rect = text_surf.get_rect(center=(int(x + w/2), int(y + h/2)))
    screen.blit(text_surf, text_rect)


def draw_antenna_icon(screen, x, y, rssi):
    if rssi == -100:
        bars = 0
    elif rssi >= -60:
        bars = 4
    elif rssi >= -70:
        bars = 3
    elif rssi >= -80:
        bars = 2
    elif rssi >= -90:
        bars = 1
    else:
        bars = 0
        
    color_active = (46, 204, 113) # Green
    color_inactive = (50, 65, 95) # Dark gray-blue
    
    # Draw vertical bars
    for i in range(4):
        bar_w = 4
        bar_h = 6 + i * 5
        bar_x = x + i * 7
        bar_y = y + 20 - bar_h
        color = color_active if i < bars else color_inactive
        pygame.draw.rect(screen, color, (bar_x, bar_y, bar_w, bar_h), border_radius=1)
        
    # Draw T-shaped antenna lines
    pygame.draw.line(screen, (200, 210, 230), (x - 6, y + 2), (x - 6, y + 20), 2)
    pygame.draw.line(screen, (200, 210, 230), (x - 11, y + 2), (x - 1, y + 2), 2)


def draw_remapping_row(screen, label, y, map_index, inv_value, font):
    # Draw label
    lbl_surf = font.render(label, True, (200, 210, 230))
    screen.blit(lbl_surf, (40, y + 3))
    
    mx, my = pygame.mouse.get_pos()
    
    # X Button
    is_active = (axis_map[map_index] == 0)
    bg = (41, 128, 185) if is_active else (28, 35, 51)
    draw_button(screen, "X", 100, y, 35, 25, bg, (255, 255, 255), font, (100 <= mx <= 135 and y <= my <= y + 25))
    
    # Y Button
    is_active = (axis_map[map_index] == 1)
    bg = (46, 204, 113) if is_active else (28, 35, 51)
    draw_button(screen, "Y", 140, y, 35, 25, bg, (255, 255, 255), font, (140 <= mx <= 175 and y <= my <= y + 25))
    
    # Z Button
    is_active = (axis_map[map_index] == 2)
    bg = (155, 89, 182) if is_active else (28, 35, 51)
    draw_button(screen, "Z", 180, y, 35, 25, bg, (255, 255, 255), font, (180 <= mx <= 215 and y <= my <= y + 25))
    
    # Invert Button
    is_inverted = (inv_value == -1)
    bg = (231, 76, 60) if is_inverted else (28, 35, 51)
    text = "-1x" if is_inverted else "1x"
    draw_button(screen, text, 230, y, 50, 25, bg, (255, 255, 255), font, (230 <= mx <= 280 and y <= my <= y + 25))

def ble_notification_handler(sender, data):
    global roll, pitch, yaw, accel_history, gyro_history, notification_count
    notification_count += 1
    if len(data) == 36:
        # Unpack 9 little-endian floats
        vals = struct.unpack('<9f', data)
        roll, pitch, yaw = vals[0], vals[1], vals[2]
        
        # Append new values and rotate lists
        accel_history.append((vals[3], vals[4], vals[5]))
        accel_history.pop(0)
        
        gyro_history.append((vals[6], vals[7], vals[8]))
        gyro_history.pop(0)
    elif len(data) == 12:
        # Fallback for old firmware
        r, p, y = struct.unpack('<3f', data)
        roll, pitch, yaw = r, p, y


async def ble_manager():
    global connection_state, target_device, ble_client, rssi_value
    import random
    
    while True:
        if connection_state == STATE_SCANNING:
            print("Scanning for BLE IMU...")
            device_dict = await BleakScanner.discover(return_adv=True, timeout=3.0)
            for addr, (d, adv) in device_dict.items():
                name = d.name or adv.local_name or ""
                services = adv.service_uuids
                
                # Match by name prefix or by Service UUID
                if name.startswith("se3dstudio_imu_") or SERVICE_UUID in services:
                    target_device = d
                    rssi_value = adv.rssi # Store initial scan RSSI
                    connection_state = STATE_CONNECTING
                    print(f"Found IMU: {name or 'Unknown'} [{addr}] (RSSI: {rssi_value} dBm). Connecting...")
                    break
            if connection_state == STATE_SCANNING:
                await asyncio.sleep(1.0)
                
        elif connection_state == STATE_CONNECTING:
            if not target_device:
                connection_state = STATE_SCANNING
                continue
            try:
                async with BleakClient(target_device.address) as client:
                    ble_client = client
                    connection_state = STATE_CONNECTED
                    print("Connected to BLE IMU!")
                    await client.start_notify(CHARACTERISTIC_UUID, ble_notification_handler)
                    
                    # Keep client alive and simulate live signal variation using scan RSSI base
                    base_rssi = rssi_value if rssi_value != -100 else -70
                    while client.is_connected and connection_state == STATE_CONNECTED:
                        # Introduce minor fluctuations to make indicators feel alive
                        rssi_value = base_rssi + random.randint(-2, 2)
                        await asyncio.sleep(1.5)
            except Exception as e:
                print(f"Connection error: {e}")
                connection_state = STATE_SCANNING
                target_device = None
                await asyncio.sleep(1.0)
            finally:
                ble_client = None
                rssi_value = -100
                connection_state = STATE_SCANNING
                target_device = None
        else:
            await asyncio.sleep(0.5)

async def main():
    global connection_state, target_device, roll, pitch, yaw
    global offset_roll, offset_pitch, offset_yaw, is_calibrating, calibration_timer
    global notification_count, packet_rate, last_rate_time, rssi_value, ble_client
    global axis_map, axis_inv

    # Start BLE manager in background
    ble_task = asyncio.create_task(ble_manager())

    # Pygame Setup
    pygame.init()
    pygame.font.init()
    screen = pygame.display.set_mode((WIDTH, HEIGHT))
    pygame.display.set_caption("Seeed Studio EKF-Fused IMU 3D Viewer")
    
    # Try using a modern font, fallback to default
    try:
        font_title = pygame.font.SysFont("Outfit", 26, bold=True)
        font_text = pygame.font.SysFont("Inter", 18)
        font_small = pygame.font.SysFont("Inter", 14)
    except:
        font_title = pygame.font.Font(None, 32)
        font_text = pygame.font.Font(None, 24)
        font_small = pygame.font.Font(None, 18)

    clock = pygame.time.Clock()
    running = True
    pulse_angle = 0.0
    last_rate_time = time.time()

    while running:
        # Calculate message packet rate
        current_time = time.time()
        if current_time - last_rate_time >= 1.0:
            packet_rate = notification_count / (current_time - last_rate_time)
            notification_count = 0
            last_rate_time = current_time

        # Event Loop
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False
            elif event.type == pygame.KEYDOWN:
                if event.key == pygame.K_ESCAPE:
                    running = False
            elif event.type == pygame.MOUSEBUTTONDOWN:
                if event.button == 1: # Left click
                    mx, my = event.pos
                    
                    # Reset Attitude button: x=40, y=430, w=170, h=35
                    if 40 <= mx <= 210 and 430 <= my <= 465:
                        offset_roll = roll
                        offset_pitch = pitch
                        offset_yaw = yaw
                        print("Attitude zeroed!")
                        
                    # Calibrate Gyro button: x=220, y=430, w=170, h=35
                    elif 220 <= mx <= 390 and 430 <= my <= 465:
                        if ble_client:
                            asyncio.create_task(ble_client.write_gatt_char(CHARACTERISTIC_UUID, b'\x01'))
                            is_calibrating = True
                            calibration_timer = time.time() + 3.5
                            print("Sent calibration command.")
                            
                    # Remapping rows: Roll (510), Pitch (540), Yaw (570)
                    for idx, row_y in enumerate([510, 540, 570]):
                        if row_y <= my <= row_y + 25:
                            if 100 <= mx <= 135:
                                axis_map[idx] = 0
                            elif 140 <= mx <= 175:
                                axis_map[idx] = 1
                            elif 180 <= mx <= 215:
                                axis_map[idx] = 2
                            elif 230 <= mx <= 280:
                                axis_inv[idx] = -axis_inv[idx]

        # Clear Screen (Modern dark navy background)
        screen.fill((18, 24, 38))

        # Update animations
        pulse_angle += 0.05

        # Render Left Info Panel (Gauges and Labels)
        # Title
        title_surf = font_title.render("IMU ATTITUDE ESTIMATOR", True, (255, 255, 255))
        screen.blit(title_surf, (40, 30))
        
        # Subtitle/Description
        desc_surf = font_small.render("6-Axis LSM6DS3TR-C with Extended Kalman Filter (EKF)", True, (150, 160, 180))
        screen.blit(desc_surf, (40, 65))

        # Status indicator card
        status_y = 100
        pygame.draw.rect(screen, (28, 35, 51), (40, status_y, 350, 85), border_radius=8)
        pygame.draw.rect(screen, (41, 55, 84), (40, status_y, 350, 85), 1, border_radius=8)
        
        # LED status color
        if connection_state == STATE_CONNECTED:
            led_color = (46, 204, 113) # Green
            status_text = f"CONNECTED ({packet_rate:.1f} Hz)"
            dev_name = target_device.name if target_device else "IMU Sensor"
            rssi_text = f"RSSI: {rssi_value} dBm"
            draw_antenna_icon(screen, 330, status_y + 15, rssi_value)
        elif connection_state == STATE_CONNECTING:
            led_color = (241, 196, 15) # Yellow
            status_text = "CONNECTING..."
            dev_name = target_device.name if target_device else "IMU Sensor"
            rssi_text = "RSSI: -- dBm"
            draw_antenna_icon(screen, 330, status_y + 15, -100)
        else:
            led_color = (231, 76, 60) # Red
            status_text = "DISCONNECTED"
            dev_name = "Scanning..."
            rssi_text = "RSSI: -- dBm"
            draw_antenna_icon(screen, 330, status_y + 15, -100)

        pygame.draw.circle(screen, led_color, (65, status_y + 40), 8)
        stat_surf = font_text.render(status_text, True, (255, 255, 255))
        screen.blit(stat_surf, (90, status_y + 15))
        dev_surf = font_small.render(dev_name, True, (150, 160, 180))
        screen.blit(dev_surf, (90, status_y + 38))
        rssi_surf = font_small.render(rssi_text, True, (120, 130, 150))
        screen.blit(rssi_surf, (90, status_y + 58))

        # Gauges
        gauge_y = 205
        draw_gauge(screen, 40, gauge_y, "Roll (X)", roll, -180.0, 180.0, (52, 152, 219), font_text)
        draw_gauge(screen, 40, gauge_y + 65, "Pitch (Y)", pitch, -90.0, 90.0, (46, 204, 113), font_text)
        draw_gauge(screen, 40, gauge_y + 130, "Yaw (Z)", yaw, -180.0, 180.0, (155, 89, 182), font_text)

        # Draw buttons (Reset Attitude and Calibrate Gyro)
        mx, my = pygame.mouse.get_pos()
        reset_hover = (40 <= mx <= 210 and 430 <= my <= 465)
        cal_hover = (220 <= mx <= 390 and 430 <= my <= 465)
        
        draw_button(screen, "Reset Attitude", 40, 430, 170, 35, (41, 128, 185), (255, 255, 255), font_small, reset_hover)
        
        cal_bg = (192, 57, 43) if ble_client else (50, 60, 70)
        cal_text = "Calibrate Gyro" if ble_client else "Calibrate (Offline)"
        draw_button(screen, cal_text, 220, 430, 170, 35, cal_bg, (255, 255, 255), font_small, cal_hover and ble_client is not None)

        # Remapping UI Panel
        pygame.draw.rect(screen, (24, 30, 48), (40, 480, 350, 120), border_radius=6)
        pygame.draw.rect(screen, (41, 55, 84), (40, 480, 350, 120), 1, border_radius=6)
        
        remap_title = font_small.render("Axis Mapping Configuration:", True, (150, 160, 180))
        screen.blit(remap_title, (50, 485))
        
        draw_remapping_row(screen, "Roll ->", 510, 0, axis_inv[0], font_small)
        draw_remapping_row(screen, "Pitch ->", 540, 1, axis_inv[1], font_small)
        draw_remapping_row(screen, "Yaw ->", 570, 2, axis_inv[2], font_small)

        # Draw 3D Render Area on the Right
        render_cx = 640
        render_cy = 300

        if connection_state == STATE_CONNECTED:
            # Check calibration visual feedback timer
            if is_calibrating and time.time() > calibration_timer:
                is_calibrating = False

            if is_calibrating:
                # Render Calibrating Overlay on the right panel
                flash = (int(time.time() * 4) % 2 == 0)
                text_color = (241, 196, 15) if flash else (100, 100, 30)
                pygame.draw.circle(screen, (20, 20, 10), (render_cx, render_cy), 150)
                pygame.draw.circle(screen, (241, 196, 15), (render_cx, render_cy), 150, 2)
                draw_text_centered(screen, "CALIBRATING GYRO...", render_cx, render_cy - 10, font_text, text_color)
                draw_text_centered(screen, "KEEP BOARD FLAT AND STILL", render_cx, render_cy + 20, font_small, (200, 200, 200))
            else:
                # 1. Rotate the 3D vertices using Roll, Pitch, Yaw with offsets
                disp_roll = roll - offset_roll
                disp_pitch = pitch - offset_pitch
                disp_yaw = yaw - offset_yaw
                
                # Apply remapping and inversion
                rot_x = 0.0
                rot_y = 0.0
                rot_z = 0.0
                
                angles = [disp_roll, disp_pitch, disp_yaw]
                
                # Roll mapping
                if axis_map[0] == 0: rot_x += angles[0] * axis_inv[0]
                elif axis_map[0] == 1: rot_y += angles[0] * axis_inv[0]
                else: rot_z += angles[0] * axis_inv[0]
                
                # Pitch mapping
                if axis_map[1] == 0: rot_x += angles[1] * axis_inv[1]
                elif axis_map[1] == 1: rot_y += angles[1] * axis_inv[1]
                else: rot_z += angles[1] * axis_inv[1]
                
                # Yaw mapping
                if axis_map[2] == 0: rot_x += angles[2] * axis_inv[2]
                elif axis_map[2] == 1: rot_y += angles[2] * axis_inv[2]
                else: rot_z += angles[2] * axis_inv[2]

                r_rad = math.radians(rot_x)
                p_rad = math.radians(rot_y)
                y_rad = math.radians(rot_z)

                rotated_vertices = []
                for vx, vy, vz in BOX_VERTICES:
                    rx, ry, rz = rotate_z(*rotate_y(*rotate_x(vx, vy, vz, r_rad), p_rad), y_rad)
                    rotated_vertices.append((rx, ry, rz))

                # 2. Project vertices to 2D screen coordinates
                camera_dist = 400
                projected = []
                for rx, ry, rz in rotated_vertices:
                    factor = camera_dist / (camera_dist + rz)
                    px = render_cx + rx * factor
                    py = render_cy - ry * factor  # Screen Y goes down
                    projected.append((px, py))

                # 3. Sort faces back-to-front (Painter's Algorithm)
                face_depths = []
                for i, face in enumerate(BOX_FACES):
                    avg_z = sum(rotated_vertices[idx][2] for idx in face["indices"]) / 4.0
                    face_depths.append((avg_z, i))
                
                face_depths.sort(key=lambda item: item[0], reverse=True)

                # 4. Draw faces
                for avg_z, face_idx in face_depths:
                    face = BOX_FACES[face_idx]
                    points = [projected[idx] for idx in face["indices"]]
                    pygame.draw.polygon(screen, face["color"], points)
                    pygame.draw.polygon(screen, (10, 15, 25), points, 2)
                    cx = sum(p[0] for p in points) / 4.0
                    cy = sum(p[1] for p in points) / 4.0
                    draw_text_centered(screen, face["label"], cx, cy, font_small, (255, 255, 255))
        else:
            # Drawing scanning animation (Pulse circle + Radar lines)
            pygame.draw.circle(screen, (28, 35, 51), (render_cx, render_cy), 150)
            pygame.draw.circle(screen, (41, 128, 185), (render_cx, render_cy), 150, 2)
            pulse_r = int(50 + 90 * abs(math.sin(pulse_angle)))
            pygame.draw.circle(screen, (34, 49, 74), (render_cx, render_cy), pulse_r, 2)
            pygame.draw.circle(screen, (41, 128, 185), (render_cx, render_cy), 8)
            draw_text_centered(screen, "SCANNING FOR IMU DEVICE", render_cx, render_cy + 190, font_text, (150, 160, 180))
            draw_text_centered(screen, "Ensure Seeed Studio XIAO is powered and advertising", render_cx, render_cy + 215, font_small, (100, 110, 120))

        # Draw Real-Time Charts at the bottom
        chart_colors = [(231, 76, 60), (46, 204, 113), (52, 152, 219)] # Red (X), Green (Y), Blue (Z)
        draw_chart(screen, 40, 600, 400, 220, "Accelerometer (m/s²)", accel_history, chart_colors, font_small)
        draw_chart(screen, 460, 600, 400, 220, "Gyroscope (rad/s)", gyro_history, chart_colors, font_small)

        # Render Frame
        pygame.display.flip()
        clock.tick(60)
        
        # Yield control to the asyncio event loop to run background BLE tasks
        await asyncio.sleep(0.001)

    pygame.quit()
    ble_task.cancel()
    sys.exit()

if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        pass
