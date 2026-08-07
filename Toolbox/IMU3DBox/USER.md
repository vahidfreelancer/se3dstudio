# Wireless EKF-Fused IMU & 3D Visualizer + BLE Microphone Audio System

This system provides real-time 3D orientation estimation, motion telemetry, and live microphone audio streaming using a **Seeed Studio XIAO nRF52840 Sense** board and a custom **Pygame/Bleak/sounddevice** PC client. The board executes a high-performance Extended Kalman Filter (EKF) on-device at 30Hz while simultaneously sampling the onboard PDM digital microphone, compressing 16kHz audio using IMA ADPCM (4:1 ratio), and transmitting both streams concurrently over Bluetooth Low Energy (BLE).

---

## 1. System Overview & Features

### Firmware (Zephyr RTOS)
- **Onboard EKF**: Fuses 3-axis gyroscope and 3-axis accelerometer data using a quaternion-based Extended Kalman Filter to output stable, drift-compensated Roll, Pitch, and Yaw angles at 30Hz.
- **Onboard PDM Digital Microphone**: Captures 16kHz audio from the MSM261D3526H digital PDM MEMS microphone on a dedicated preemptible worker thread.
- **IMA ADPCM Audio Compression**: Compresses 256 16-bit PCM audio samples (512 bytes) into 128 bytes of IMA ADPCM data ($4:1$ compression ratio), reducing BLE packet overhead and preventing buffer congestion.
- **BLE Flow Control & CCCD Gating**: Audio sampling and transmission are automatically gated by GATT CCCD subscription status, with flow control backoff handling and 32 ACL TX buffers to prevent memory slab exhaustion.
- **Dynamic BLE Service**: Advertises as `se3dstudio_imu_[XXXX]` (or `se3dstudio_imu_01`) containing a custom GATT service with IMU (`...ef1`) and Audio (`...ef2`) characteristics.
- **Auto-Advertising**: Automatically restarts BLE advertising via system workqueue when a client disconnects, allowing seamless instant reconnections without needing hardware reboots.
- **Device-Side Calibration**: Accepts a write command (`0x01`) from the client to calibrate gyroscope biases dynamically when the board is still.
- **Visual Feedback**: Toggles onboard RGB LED colors (Red = disconnected/advertising, Blue = connected, blinking Yellow = calibrating, Green flash = calibration complete).

### PC Visualizer (Pygame & sounddevice Client)
- **3D Cuboid Rendering**: Perspective-projected 3D model of the Seeed Studio board rotating in real time at 30Hz.
- **Real-Time Audio Waveform & Speaker Playback**: Decodes incoming IMA ADPCM audio packets, plots a real-time oscilloscope waveform graph, and outputs live 16kHz audio to PC speakers via `sounddevice`.
- **Digital Audio Gain Amplification**: Applies $32\times$ digital gain to raw MEMS microphone samples for clear visual amplitude representation and speaker volume.
- **Real-Time Telemetry**: Monitors connection packet rates (Hz), RSSI (dBm), and displays a graphic antenna signal strength icon.
- **Attitude Centering**: Features a `Reset Attitude` button to re-zero the orientation offsets instantly.
- **Gyro Calibration Control**: Features a `Calibrate Gyro` button that commands the board to perform hardware-level bias calculations.
- **Interactive Remapping**: Features controls to remap Roll, Pitch, and Yaw to the visualizer's X, Y, and Z rotation axes, with independent inversion controls.
- **Real-Time Sensor Charts**: Plots Accelerometer ($m/s^2$), Gyroscope ($rad/s$), and Microphone Audio Waveform (PCM) on scrolling charts.

---

## 2. Setup & Usage Instructions

### Prerequisite Dependencies
Ensure Python 3 (3.10+) is installed with the required libraries:
```bash
pip install pygame bleak sounddevice numpy
```

### Step 1: Flash Firmware
1. Connect the Seeed Studio XIAO board to your PC via a USB-C cable.
2. **Double-click the reset button** on the board (next to the USB connector). The board's LED will pulse green, and it will mount as a USB storage drive named `XIAO-SENSE` (`E:\`).
3. Copy the compiled binary to the drive:
   ```powershell
   Copy-Item build/app_ble_sensor/zephyr/zephyr.uf2 -Destination E:\ -Force
   ```
4. The board will automatically reboot, flash itself, and start blinking Red.

### Step 2: Run the 3D Visualizer & Audio Dashboard
In your PC terminal, run:
```bash
python Toolbox/IMU3DBox/imu_3d_viewer.py
```
The application will automatically scan for `se3dstudio_imu_01`, connect, subscribe to both IMU and Audio streams, and start rendering the 3D box, live audio waveform, and speaker output.

---

## 3. UI Controls Reference

- **Reset Attitude**: Click to snap the current board orientation to "flat" ($0^\circ, 0^\circ, 0^\circ$).
- **Calibrate Gyro**: Click to run the hardware calibration. **Ensure the board is kept completely flat and stationary** for ~3.5 seconds while the board LED flashes yellow.
- **Axis Mapping Configuration**:
  - Click `X`, `Y`, or `Z` on any row to route input angles to the box's corresponding rotation axes.
  - Click `1x` / `-1x` to invert the rotation directions.

---

## 4. Technical Protocol Details

### BLE UUIDs
- **Custom Service**: `12345678-1234-5678-1234-56789abcdef0`
- **IMU Characteristic (Notify/Write)**: `12345678-1234-5678-1234-56789abcdef1`
- **Audio Characteristic (Notify)**: `12345678-1234-5678-1234-56789abcdef2`

### Data Packet Layouts (GATT Notifications)

#### IMU Notification Payload (36 Bytes, 9 Floats)
The IMU notification sends a **36-byte** payload consisting of **9 little-endian IEEE-754 floats** (`<9f`):

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

#### Audio Notification Payload (128 Bytes, IMA ADPCM)
- Contains **128 bytes** representing 256 16-bit PCM audio samples compressed using IMA ADPCM (4-bit nibbles).
- Sampling Rate: 16,000 Hz (Mono, 16-bit PCM).

### GATT Write Command
- Send `0x01` to `...ef1` to trigger gyro bias calibration.
