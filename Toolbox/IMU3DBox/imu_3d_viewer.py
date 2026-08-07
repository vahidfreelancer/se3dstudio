import asyncio
import struct
import sys
import math
import pygame
import time
import random
import queue
from bleak import BleakScanner, BleakClient

# Try importing sounddevice and numpy for low-latency audio playback
try:
    import sounddevice as sd
    import numpy as np
    HAS_SOUNDDEVICE = True
except ImportError:
    HAS_SOUNDDEVICE = False

# BLE Custom Service & Characteristic UUIDs matching firmware
SERVICE_UUID = "12345678-1234-5678-1234-56789abcdef0"
CHARACTERISTIC_UUID = "12345678-1234-5678-1234-56789abcdef1"
AUDIO_CHARACTERISTIC_UUID = "12345678-1234-5678-1234-56789abcdef2"

# Digital Audio Gain Multiplier (Amplify raw PDM microphone signal)
AUDIO_GAIN = 8.0

# IMA ADPCM State & Tables
indexTable = [-1, -1, -1, -1, 2, 4, 6, 8, -1, -1, -1, -1, 2, 4, 6, 8]
stepsizeTable = [
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17,
    19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
    50, 55, 60, 66, 73, 80, 88, 97, 107, 118,
    130, 143, 157, 173, 190, 209, 230, 253, 279, 307,
    337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
    876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066,
    2272, 2499, 2749, 3024, 3326, 3659, 4025, 4428, 4871, 5358,
    5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
    15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
]

adpcm_decode_state = {'valprev': 0, 'index': 0}

def adpcm_decode_sample(code, state):
    step = stepsizeTable[state['index']]
    vpdiff = step >> 3
    if code & 4: vpdiff += step
    if code & 2: vpdiff += step >> 1
    if code & 1: vpdiff += step >> 2
    if code & 8: state['valprev'] -= vpdiff
    else: state['valprev'] += vpdiff
    state['valprev'] = max(-32768, min(32767, state['valprev']))
    state['index'] += indexTable[code & 7]
    state['index'] = max(0, min(88, state['index']))
    return state['valprev']

def adpcm_decode_block(encoded_bytes, state):
    pcm_samples = []
    for b in encoded_bytes:
        code1 = b & 0x0F
        code2 = (b >> 4) & 0x0F
        s1 = adpcm_decode_sample(code1, state)
        s2 = adpcm_decode_sample(code2, state)
        pcm_samples.append(s1)
        pcm_samples.append(s2)
    return pcm_samples

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

# Axis remapping variables
axis_map = [0, 1, 2]
axis_inv = [1, 1, 1]

# Screen dimensions
WIDTH, HEIGHT = 1280, 850

# Real-time sensor buffers for charts
HISTORY_LEN = 100
accel_history = [(0.0, 0.0, 0.0)] * HISTORY_LEN
gyro_history = [(0.0, 0.0, 0.0)] * HISTORY_LEN

AUDIO_HISTORY_LEN = 400
audio_history = [0.0] * AUDIO_HISTORY_LEN

# Audio Queue for real-time sounddevice playback
audio_queue = queue.Queue(maxsize=100)

def sd_audio_callback(outdata, frames, time_info, status):
    needed_bytes = frames * 2
    collected = bytearray()
    while len(collected) < needed_bytes:
        try:
            chunk = audio_queue.get_nowait()
            collected.extend(chunk)
        except queue.Empty:
            break
    if len(collected) < needed_bytes:
        collected.extend(b'\x00' * (needed_bytes - len(collected)))
    
    pcm_data = np.frombuffer(collected[:needed_bytes], dtype=np.int16)
    audio_float = (pcm_data.astype(np.float32) * AUDIO_GAIN) / 32768.0
    audio_float = np.clip(audio_float, -1.0, 1.0)
    outdata[:, 0] = audio_float

# 3D Box vertices (representing Seeed Studio XIAO board)
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

# 3D Box faces
BOX_FACES = [
    {"indices": [4, 5, 6, 7], "color": (41, 128, 185), "label": "TOP (Sense)"},
    {"indices": [0, 3, 2, 1], "color": (34, 49, 74),   "label": "BOTTOM"},
    {"indices": [0, 1, 5, 4], "color": (52, 73, 94),   "label": "FRONT"},
    {"indices": [2, 3, 7, 6], "color": (44, 62, 80),   "label": "BACK"},
    {"indices": [0, 4, 7, 3], "color": (26, 188, 156), "label": "LEFT"},
    {"indices": [1, 2, 6, 5], "color": (22, 160, 133), "label": "RIGHT"}
]

def rotate_x(x, y, z, angle):
    rad = angle
    cos_a, sin_a = math.cos(rad), math.sin(rad)
    return x, y * cos_a - z * sin_a, y * sin_a + z * cos_a

def rotate_y(x, y, z, angle):
    rad = angle
    cos_a, sin_a = math.cos(rad), math.sin(rad)
    return x * cos_a + z * sin_a, y, -x * sin_a + z * cos_a

def rotate_z(x, y, z, angle):
    rad = angle
    cos_a, sin_a = math.cos(rad), math.sin(rad)
    return x * cos_a - y * sin_a, x * sin_a + y * cos_a, z

def draw_text_centered(screen, text, x, y, font, color=(255, 255, 255)):
    text_surface = font.render(text, True, color)
    text_rect = text_surface.get_rect(center=(int(x), int(y)))
    screen.blit(text_surface, text_rect)

def draw_gauge(screen, x, y, label, val, min_val, max_val, color, font):
    width = 200
    height = 10
    pygame.draw.rect(screen, (50, 50, 50), (x, y, width, height), border_radius=5)
    clamped_val = max(min_val, min(max_val, val))
    pct = (clamped_val - min_val) / (max_val - min_val)
    fill_width = int(pct * width)
    pygame.draw.rect(screen, color, (x, y, fill_width, height), border_radius=5)
    lbl_surf = font.render(f"{label}: {val:6.1f}°", True, (240, 240, 240))
    screen.blit(lbl_surf, (x, y - 25))

def draw_chart(screen, x, y, w, h, title, history, colors, font):
    pygame.draw.rect(screen, (24, 30, 48), (x, y, w, h), border_radius=8)
    pygame.draw.rect(screen, (41, 55, 84), (x, y, w, h), 2, border_radius=8)
    
    title_surf = font.render(title, True, (200, 210, 230))
    screen.blit(title_surf, (x + 10, y + 8))
    
    points_x, points_y, points_z = [], [], []
    dx = (w - 20) / (HISTORY_LEN - 1)
    
    actual_min = min(min(pt[0] for pt in history), min(pt[1] for pt in history), min(pt[2] for pt in history))
    actual_max = max(max(pt[0] for pt in history), max(pt[1] for pt in history), max(pt[2] for pt in history))
    
    span = max(abs(actual_min), abs(actual_max))
    if span < 1.0:
        span = 1.0
    
    cur_y_min, cur_y_max = -span * 1.1, span * 1.1
    
    zero_pct = (0 - cur_y_min) / (cur_y_max - cur_y_min)
    zero_y = y + h - int(zero_pct * (h - 45)) - 15
    pygame.draw.line(screen, (50, 65, 95), (x + 10, zero_y), (x + w - 10, zero_y), 1)

    for i, (vx, vy, vz) in enumerate(history):
        px = x + 10 + i * dx
        def val_to_y(v):
            pct = (v - cur_y_min) / (cur_y_max - cur_y_min)
            return y + h - int(pct * (h - 45)) - 15
            
        points_x.append((px, val_to_y(vx)))
        points_y.append((px, val_to_y(vy)))
        points_z.append((px, val_to_y(vz)))
        
    if len(points_x) > 1:
        pygame.draw.lines(screen, colors[0], False, points_x, 2)
        pygame.draw.lines(screen, colors[1], False, points_y, 2)
        pygame.draw.lines(screen, colors[2], False, points_z, 2)
        
    latest = history[-1]
    lbl_x = font.render(f"X:{latest[0]:.1f}", True, colors[0])
    lbl_y = font.render(f"Y:{latest[1]:.1f}", True, colors[1])
    lbl_z = font.render(f"Z:{latest[2]:.1f}", True, colors[2])
    
    screen.blit(lbl_x, (x + w - 160, y + 8))
    screen.blit(lbl_y, (x + w - 110, y + 8))
    screen.blit(lbl_z, (x + w - 60, y + 8))

def draw_audio_chart(screen, x, y, w, h, title, history, color, font):
    pygame.draw.rect(screen, (24, 30, 48), (x, y, w, h), border_radius=8)
    pygame.draw.rect(screen, (41, 55, 84), (x, y, w, h), 2, border_radius=8)
    
    title_surf = font.render(title, True, (200, 210, 230))
    screen.blit(title_surf, (x + 10, y + 8))
    
    mid_y = y + h // 2 + 10
    pygame.draw.line(screen, (50, 65, 95), (x + 10, mid_y), (x + w - 10, mid_y), 1)

    points = []
    dx = (w - 20) / (len(history) - 1) if len(history) > 1 else 1
    
    for i, val in enumerate(history):
        px = x + 10 + i * dx
        cval = max(-1.0, min(1.0, val))
        py = mid_y - int(cval * (h / 2 - 25))
        points.append((px, py))

    if len(points) > 1:
        pygame.draw.lines(screen, color, False, points, 2)
        
    latest = history[-1] if history else 0.0
    lbl = font.render(f"ADPCM 16kHz | Pcm: {latest:+.2f}", True, color)
    screen.blit(lbl, (x + w - 180, y + 8))

def draw_button(screen, text, x, y, w, h, bg_color, text_color, font, is_hovered):
    color = (min(255, bg_color[0] + 25), min(255, bg_color[1] + 25), min(255, bg_color[2] + 25)) if is_hovered else bg_color
    pygame.draw.rect(screen, color, (x, y, w, h), border_radius=5)
    pygame.draw.rect(screen, (70, 90, 130), (x, y, w, h), 1, border_radius=5)
    text_surf = font.render(text, True, text_color)
    text_rect = text_surf.get_rect(center=(int(x + w/2), int(y + h/2)))
    screen.blit(text_surf, text_rect)

def draw_antenna_icon(screen, x, y, rssi):
    bars = 0 if rssi == -100 else (4 if rssi >= -60 else (3 if rssi >= -70 else (2 if rssi >= -80 else (1 if rssi >= -90 else 0))))
    color_active = (46, 204, 113)
    color_inactive = (60, 70, 90)
    for i in range(4):
        bar_x = x + i * 6
        bar_h = 4 + i * 4
        bar_y = y + 20 - bar_h
        color = color_active if i < bars else color_inactive
        pygame.draw.rect(screen, color, (bar_x, bar_y, 4, bar_h), border_radius=1)
    pygame.draw.line(screen, (200, 210, 230), (x - 6, y + 2), (x - 6, y + 20), 2)
    pygame.draw.line(screen, (200, 210, 230), (x - 11, y + 2), (x - 1, y + 2), 2)

def draw_remapping_row(screen, label, y, map_index, inv_value, font):
    lbl_surf = font.render(label, True, (200, 210, 230))
    screen.blit(lbl_surf, (40, y + 3))
    mx, my = pygame.mouse.get_pos()
    
    bg = (41, 128, 185) if axis_map[map_index] == 0 else (28, 35, 51)
    draw_button(screen, "X", 100, y, 35, 25, bg, (255, 255, 255), font, (100 <= mx <= 135 and y <= my <= y + 25))
    
    bg = (46, 204, 113) if axis_map[map_index] == 1 else (28, 35, 51)
    draw_button(screen, "Y", 140, y, 35, 25, bg, (255, 255, 255), font, (140 <= mx <= 175 and y <= my <= y + 25))
    
    bg = (155, 89, 182) if axis_map[map_index] == 2 else (28, 35, 51)
    draw_button(screen, "Z", 180, y, 35, 25, bg, (255, 255, 255), font, (180 <= mx <= 215 and y <= my <= y + 25))
    
    is_inverted = (inv_value == -1)
    bg = (231, 76, 60) if is_inverted else (28, 35, 51)
    text = "-1x" if is_inverted else "1x"
    draw_button(screen, text, 230, y, 50, 25, bg, (255, 255, 255), font, (230 <= mx <= 280 and y <= my <= y + 25))

def ble_notification_handler(sender, data):
    global roll, pitch, yaw, accel_history, gyro_history, notification_count
    notification_count += 1
    if len(data) == 36:
        vals = struct.unpack('<9f', data)
        roll, pitch, yaw = vals[0], vals[1], vals[2]
        
        accel_history.append((vals[3], vals[4], vals[5]))
        accel_history.pop(0)
        
        gyro_history.append((vals[6], vals[7], vals[8]))
        gyro_history.pop(0)

def audio_notification_handler(sender, data):
    global audio_history, adpcm_decode_state
    if len(data) > 0:
        # Decode IMA ADPCM 4-bit bytes to 16-bit PCM samples
        pcm_samples = adpcm_decode_block(data, adpcm_decode_state)
        
        norm_samples = [min(1.0, max(-1.0, (s * AUDIO_GAIN) / 32768.0)) for s in pcm_samples]
        audio_history.extend(norm_samples)
        if len(audio_history) > AUDIO_HISTORY_LEN:
            audio_history = audio_history[-AUDIO_HISTORY_LEN:]
            
        raw_pcm_bytes = struct.pack(f'<{len(pcm_samples)}h', *pcm_samples)
        try:
            audio_queue.put_nowait(raw_pcm_bytes)
        except queue.Full:
            pass

async def ble_manager():
    global connection_state, target_device, ble_client, rssi_value, adpcm_decode_state
    
    while True:
        if connection_state == STATE_SCANNING:
            print("Scanning for BLE IMU + Audio...")
            device_dict = await BleakScanner.discover(return_adv=True, timeout=3.0)
            for addr, (d, adv) in device_dict.items():
                name = d.name or adv.local_name or ""
                services = adv.service_uuids
                
                if name.startswith("se3dstudio_imu_") or SERVICE_UUID in services:
                    target_device = d
                    rssi_value = adv.rssi
                    connection_state = STATE_CONNECTING
                    print(f"Found Device: {name or 'Unknown'} [{addr}] (RSSI: {rssi_value} dBm). Connecting...")
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
                    adpcm_decode_state['valprev'] = 0
                    adpcm_decode_state['index'] = 0
                    print("Connected to BLE IMU + Audio!")
                    await client.start_notify(CHARACTERISTIC_UUID, ble_notification_handler)
                    try:
                        await client.start_notify(AUDIO_CHARACTERISTIC_UUID, audio_notification_handler)
                        print("Subscribed to ADPCM Audio Stream!")
                    except Exception as ea:
                        print(f"Audio notification subscription notice: {ea}")

                    base_rssi = rssi_value if rssi_value != -100 else -70
                    while client.is_connected and connection_state == STATE_CONNECTED:
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

    ble_task = asyncio.create_task(ble_manager())

    pygame.init()
    pygame.font.init()

    # Start sounddevice audio output stream if available
    sd_stream = None
    if HAS_SOUNDDEVICE:
        try:
            sd_stream = sd.OutputStream(samplerate=16000, channels=1, callback=sd_audio_callback, blocksize=256)
            sd_stream.start()
            print("Sounddevice real-time audio output stream started at 16kHz!")
        except Exception as esd:
            print(f"Sounddevice stream notice: {esd}")

    screen = pygame.display.set_mode((WIDTH, HEIGHT))
    pygame.display.set_caption("Seeed Studio EKF IMU 3D Viewer & BLE Audio Visualizer")
    
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
        current_time = time.time()
        if current_time - last_rate_time >= 1.0:
            packet_rate = notification_count / (current_time - last_rate_time)
            notification_count = 0
            last_rate_time = current_time

        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False
            elif event.type == pygame.KEYDOWN:
                if event.key == pygame.K_ESCAPE:
                    running = False
            elif event.type == pygame.MOUSEBUTTONDOWN:
                if event.button == 1:
                    mx, my = event.pos
                    
                    if 40 <= mx <= 210 and 430 <= my <= 465:
                        offset_roll = roll
                        offset_pitch = pitch
                        offset_yaw = yaw
                        print("Attitude zeroed!")
                        
                    elif 220 <= mx <= 390 and 430 <= my <= 465:
                        if ble_client:
                            asyncio.create_task(ble_client.write_gatt_char(CHARACTERISTIC_UUID, b'\x01'))
                            is_calibrating = True
                            calibration_timer = time.time() + 3.5
                            print("Sent calibration command.")
                            
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

        screen.fill((18, 24, 38))
        pulse_angle += 0.05

        title_surf = font_title.render("IMU ATTITUDE & BLE AUDIO VISUALIZER", True, (255, 255, 255))
        screen.blit(title_surf, (40, 30))
        
        desc_surf = font_small.render("6-Axis LSM6DS3TR-C EKF + Onboard PDM Digital Microphone", True, (150, 160, 180))
        screen.blit(desc_surf, (40, 65))

        status_y = 100
        pygame.draw.rect(screen, (28, 35, 51), (40, status_y, 350, 85), border_radius=8)
        pygame.draw.rect(screen, (41, 55, 84), (40, status_y, 350, 85), 1, border_radius=8)
        
        if connection_state == STATE_CONNECTED:
            led_color = (46, 204, 113)
            status_text = f"CONNECTED ({packet_rate:.1f} Hz)"
            dev_name = target_device.name if target_device else "IMU Sensor"
            rssi_text = f"RSSI: {rssi_value} dBm"
            draw_antenna_icon(screen, 330, status_y + 15, rssi_value)
        elif connection_state == STATE_CONNECTING:
            led_color = (241, 196, 15)
            status_text = "CONNECTING..."
            dev_name = target_device.name if target_device else "IMU Sensor"
            rssi_text = "RSSI: -- dBm"
            draw_antenna_icon(screen, 330, status_y + 15, -100)
        else:
            led_color = (231, 76, 60)
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

        gauge_y = 205
        draw_gauge(screen, 40, gauge_y, "Roll (X)", roll, -180.0, 180.0, (52, 152, 219), font_text)
        draw_gauge(screen, 40, gauge_y + 65, "Pitch (Y)", pitch, -90.0, 90.0, (46, 204, 113), font_text)
        draw_gauge(screen, 40, gauge_y + 130, "Yaw (Z)", yaw, -180.0, 180.0, (155, 89, 182), font_text)

        mx, my = pygame.mouse.get_pos()
        reset_hover = (40 <= mx <= 210 and 430 <= my <= 465)
        cal_hover = (220 <= mx <= 390 and 430 <= my <= 465)
        
        draw_button(screen, "Reset Attitude", 40, 430, 170, 35, (41, 128, 185), (255, 255, 255), font_small, reset_hover)
        
        cal_bg = (192, 57, 43) if ble_client else (50, 60, 70)
        cal_text = "Calibrate Gyro" if ble_client else "Calibrate (Offline)"
        draw_button(screen, cal_text, 220, 430, 170, 35, cal_bg, (255, 255, 255), font_small, cal_hover and ble_client is not None)

        pygame.draw.rect(screen, (24, 30, 48), (40, 480, 350, 120), border_radius=6)
        pygame.draw.rect(screen, (41, 55, 84), (40, 480, 350, 120), 1, border_radius=6)
        
        remap_title = font_small.render("Axis Mapping Configuration:", True, (150, 160, 180))
        screen.blit(remap_title, (50, 485))
        
        draw_remapping_row(screen, "Roll ->", 510, 0, axis_inv[0], font_small)
        draw_remapping_row(screen, "Pitch ->", 540, 1, axis_inv[1], font_small)
        draw_remapping_row(screen, "Yaw ->", 570, 2, axis_inv[2], font_small)

        render_cx = 830
        render_cy = 300

        if connection_state == STATE_CONNECTED:
            if is_calibrating and time.time() > calibration_timer:
                is_calibrating = False

            if is_calibrating:
                cal_r = int(120 + 20 * math.sin(pulse_angle * 3))
                pygame.draw.circle(screen, (231, 76, 60), (render_cx, render_cy), cal_r, 3)
                draw_text_centered(screen, "GYRO CALIBRATING...", render_cx, render_cy, font_title, (241, 196, 15))
                draw_text_centered(screen, "Keep board still on flat surface", render_cx, render_cy + 30, font_small, (200, 200, 200))
            else:
                angles = [roll - offset_roll, pitch - offset_pitch, yaw - offset_yaw]
                rot_x, rot_y, rot_z = 0.0, 0.0, 0.0
                
                if axis_map[0] == 0: rot_x += angles[0] * axis_inv[0]
                elif axis_map[0] == 1: rot_y += angles[0] * axis_inv[0]
                else: rot_z += angles[0] * axis_inv[0]
                
                if axis_map[1] == 0: rot_x += angles[1] * axis_inv[1]
                elif axis_map[1] == 1: rot_y += angles[1] * axis_inv[1]
                else: rot_z += angles[1] * axis_inv[1]
                
                if axis_map[2] == 0: rot_x += angles[2] * axis_inv[2]
                elif axis_map[2] == 1: rot_y += angles[2] * axis_inv[2]
                else: rot_z += angles[2] * axis_inv[2]

                r_rad, p_rad, y_rad = math.radians(rot_x), math.radians(rot_y), math.radians(rot_z)

                rotated_vertices = []
                for vx, vy, vz in BOX_VERTICES:
                    rx, ry, rz = rotate_z(*rotate_y(*rotate_x(vx, vy, vz, r_rad), p_rad), y_rad)
                    rotated_vertices.append((rx, ry, rz))

                camera_dist = 400
                projected = []
                for rx, ry, rz in rotated_vertices:
                    factor = camera_dist / (camera_dist + rz)
                    px = render_cx + rx * factor
                    py = render_cy - ry * factor
                    projected.append((px, py))

                face_depths = []
                for i, face in enumerate(BOX_FACES):
                    avg_z = sum(rotated_vertices[idx][2] for idx in face["indices"]) / 4.0
                    face_depths.append((avg_z, i))
                
                face_depths.sort(key=lambda item: item[0], reverse=True)

                for avg_z, face_idx in face_depths:
                    face = BOX_FACES[face_idx]
                    points = [projected[idx] for idx in face["indices"]]
                    pygame.draw.polygon(screen, face["color"], points)
                    pygame.draw.polygon(screen, (10, 15, 25), points, 2)
                    cx = sum(p[0] for p in points) / 4.0
                    cy = sum(p[1] for p in points) / 4.0
                    draw_text_centered(screen, face["label"], cx, cy, font_small, (255, 255, 255))
        else:
            pygame.draw.circle(screen, (28, 35, 51), (render_cx, render_cy), 150)
            pygame.draw.circle(screen, (41, 128, 185), (render_cx, render_cy), 150, 2)
            pulse_r = int(50 + 90 * abs(math.sin(pulse_angle)))
            pygame.draw.circle(screen, (34, 49, 74), (render_cx, render_cy), pulse_r, 2)
            pygame.draw.circle(screen, (41, 128, 185), (render_cx, render_cy), 8)
            draw_text_centered(screen, "SCANNING FOR IMU + AUDIO DEVICE", render_cx, render_cy + 190, font_text, (150, 160, 180))
            draw_text_centered(screen, "Ensure Seeed Studio XIAO is powered and advertising", render_cx, render_cy + 215, font_small, (100, 110, 120))

        # Draw 3 Real-Time Charts at the bottom
        chart_colors = [(231, 76, 60), (46, 204, 113), (52, 152, 219)]
        draw_chart(screen, 40, 610, 380, 210, "Accelerometer (m/s²)", accel_history, chart_colors, font_small)
        draw_chart(screen, 440, 610, 380, 210, "Gyroscope (rad/s)", gyro_history, chart_colors, font_small)
        draw_audio_chart(screen, 840, 610, 400, 210, "Microphone Audio Waveform (16-bit PCM)", audio_history, (0, 230, 255), font_small)

        pygame.display.flip()
        clock.tick(60)
        await asyncio.sleep(0.001)

    if sd_stream:
        sd_stream.stop()
        sd_stream.close()
    pygame.quit()
    ble_task.cancel()
    sys.exit()

if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        pass
