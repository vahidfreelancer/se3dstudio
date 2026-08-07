# Wireless EKF-Fused IMU & 3D Visualizer System

This system provides real-time 3D orientation estimation and motion telemetry using a **Seeed Studio XIAO nRF52840 Sense** board and a custom **Pygame/Bleak** PC client. The board executes a high-performance Extended Kalman Filter (EKF) on-device and streams attitude angles along with raw accelerometer/gyroscope readings over Bluetooth Low Energy (BLE) at 30Hz.

---

## 1. System Overview & Features

### Firmware (Zephyr RTOS)
- **Onboard EKF**: Fuses 3-axis gyroscope and 3-axis accelerometer data using a quaternion-based Extended Kalman Filter to output stable, drift-compensated Roll, Pitch, and Yaw angles.
- **Dynamic BLE Service**: Advertises as `se3dstudio_imu_[XXXX]` containing a custom GATT service and notifying a 36-byte payload (9 floats).
- **Auto-Advertising**: Automatically restarts BLE advertising when a client disconnects, allowing seamless reconnections.
- **Device-Side Calibration**: Accepts a write command (`0x01`) from the client to calibrate gyroscope biases dynamically when the board is still.
- **Visual Feedback**: Toggles onboard RGB LED colors (Red = disconnected/advertising, Blue = connected, blinking Yellow = calibrating, Green flash = calibration complete).

### PC Visualizer (Pygame Client)
- **3D Cuboid Rendering**: Perspective-projected 3D model of the Seeed Studio board rotating in real time.
- **Real-Time Telemetry**: Monitors connection packet rates (Hz), RSSI (dBm), and displays a graphic antenna signal strength icon.
- **Attitude Centering**: Features a `Reset Attitude` button to re-zero the orientation offsets instantly.
- **Gyro Calibration Control**: Features a `Calibrate Gyro` button that commands the board to perform hardware-level bias calculations.
- **Interactive Remapping**: Features controls to remap Roll, Pitch, and Yaw to the visualizer's X, Y, and Z rotation axes, with independent inversion controls.
- **Real-Time Charts**: Plots 3-axis Accelerometer ($m/s^2$) and Gyroscope ($rad/s$) values on dual real-time scrolling charts (Red = X, Green = Y, Blue = Z).

---

## 2. Setup & Usage Instructions

### Prerequisite Dependencies
Ensure Python 3 (3.10+) is installed with the required libraries. Run:
```bash
pip install pygame bleak
```

### Step 1: Flash the Firmware
1. Connect the Seeed Studio XIAO board to your PC via a USB-C cable.
2. **Double-click the reset button** on the board (next to the USB connector). The board's LED will pulse green, and it will mount as a USB storage drive named `XIAO-SENSE`.
3. Copy the compiled binary to the drive:
   ```bash
   cp build/app_ble_sensor/zephyr/zephyr.uf2 /run/media/vahid/XIAO-SENSE/ && sync
   ```
4. The board will automatically reboot, flash itself, and start blinking Red.

### Step 2: Run the 3D Visualizer
In your PC terminal, run:
```bash
cd Toolbox/IMU3DBox
python3 imu_3d_viewer.py
```
The application will automatically scan for the board, connect, and start displaying the 3D box and data charts.

---

## 3. UI Controls Reference

- **Reset Attitude**: Click to snap the current board orientation to "flat" ($0^\circ, 0^\circ, 0^\circ$).
- **Calibrate Gyro**: Click to run the hardware calibration. **Ensure the board is kept completely flat and stationary** for the ~3 seconds when the board LED flashes yellow.
- **Axis Mapping Configuration**:
  - Click `X`, `Y`, or `Z` on any row to route input angles to the box's corresponding rotation axes.
  - Click `1x` / `-1x` to invert the rotation directions.

---

## 4. Technical Protocol Details

### BLE UUIDs
- **Custom Service**: `12345678-1234-5678-1234-56789abcdef0`
- **Custom Characteristic (Notify/Write)**: `12345678-1234-5678-1234-56789abcdef1`

### Data Packet Layout (GATT Notification)
The notification sends a **36-byte** payload consisting of **9 little-endian IEEE-754 floats** (`<9f`):

| Offset (Bytes) | Type | Variable | Unit | Description |
|---|---|---|---|---|
| `0 - 3` | `float` | `Roll` | Degrees ($^\circ$) | EKF fused roll angle |
| `4 - 7` | `float` | `Pitch` | Degrees ($^\circ$) | EKF fused pitch angle |
| `8 - 11` | `float` | `Yaw` | Degrees ($^\circ$) | EKF fused yaw angle |
| `12 - 15` | `float` | `Accel X` | $m/s^2$ | Raw accelerometer X |
| `16 - 19` | `float` | `Accel Y` | $m/s^2$ | Raw accelerometer Y |
| `20 - 23` | `float` | `Accel Z` | $m/s^2$ | Raw accelerometer Z |
| `24 - 27` | `float` | `Gyro X` | $rad/s$ | Bias-corrected gyroscope X |
| `28 - 31` | `float` | `Gyro Y` | $rad/s$ | Bias-corrected gyroscope Y |
| `32 - 35` | `float` | `Gyro Z` | $rad/s$ | Bias-corrected gyroscope Z |

### GATT Write Command
- Send `0x01` to trigger gyro bias calibration.
