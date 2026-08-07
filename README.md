# Seeed Studio XIAO nRF52840 Sense Platform

[![Zephyr RTOS](https://img.shields.io/badge/Zephyr_RTOS-v3.2+-blue.svg)](https://www.zephyrproject.org/)
[![nRF Connect SDK](https://img.shields.io/badge/nRF_Connect_SDK-v2.x%2Fv3.x-brightgreen.svg)](https://www.nordicsemi.com/Products/Development-software/nrf-connect-sdk)
[![Hardware](https://img.shields.io/badge/Hardware-Seeed_XIAO_nRF52840_Sense-orange.svg)](https://www.seeedstudio.com/XIAO-BLE-Sense-nRF52840-p-5253.html)
[![License](https://img.shields.io/badge/License-MIT-purple.svg)](LICENSE)

A modular, production-ready **Zephyr RTOS** embedded framework designed as a general-purpose platform for the **Seeed Studio XIAO nRF52840 Sense Core**. 

This monorepo codebase decouples hardware drivers (`shared/drivers/`) from business logic and isolated applications (`apps/`), featuring real-time Extended Kalman Filter (EKF) 3D attitude estimation, PDM digital MEMS microphone capture, IMA ADPCM audio compression, and low-latency Bluetooth Low Energy (BLE) streaming paired with interactive Python telemetry tools.

---

## 🚀 Key Platform Features

- **Decoupled Architecture**: Shared hardware abstraction drivers (`shared/drivers/`) and business logic libraries (`shared/lib/`) accessible across isolated application targets (`apps/`).
- **Real-Time Onboard EKF**: Quaternion-based Extended Kalman Filter running on-device at 30Hz for 6-axis IMU (LSM6DS3TR-C) gyro bias compensation and drift-free Roll/Pitch/Yaw estimation.
- **PDM Digital Audio Pipeline**: 16kHz audio sampling from the onboard MSM261D3526H MEMS microphone executed on a dedicated preemptible worker thread.
- **IMA ADPCM 4:1 Compression**: Real-time on-device audio compression reducing 16-bit PCM (512 bytes) to 128-byte packets, enabling reliable dual-stream IMU + Audio transmission without BLE buffer congestion.
- **BLE Flow Control & Auto-Advertising**: 32-packet ACL TX buffer pool, GATT CCCD notification gating, and automatic advertising restart via Zephyr system workqueues upon disconnect.
- **Python 3D Telemetry & Audio Visualizer**: Real-time Pygame 3D box renderer, dual-axis motion charts, oscilloscope waveform display, and low-latency speaker playback using `sounddevice`.

---

## 🛠️ Hardware Specifications

| Component | Specification |
|---|---|
| **MCU Core** | Nordic Semiconductor nRF52840 (ARM Cortex-M4F @ 64 MHz) |
| **Memory** | 1 MB Internal Flash, 256 KB RAM |
| **6-Axis IMU** | LSM6DS3TR-C (3-axis Accelerometer + 3-axis Gyroscope via I2C) |
| **Digital Microphone** | MSM261D3526H PDM Digital MEMS Microphone |
| **Wireless** | Bluetooth 5.0 LE, 2.4 GHz Antenna |
| **Peripherals** | Onboard Battery Charge IC, RGB LED, UF2 Bootloader |

---

## 📂 Repository Structure

```text
├── apps/                          # Isolated Application Modules
│   ├── app_ble_sensor/            # Main Dual-Stream EKF IMU + ADPCM Audio App
│   ├── app_template/              # Boilerplate starter template for new apps
│   ├── Ble-mic/                   # Standalone Bluetooth PDM mic recorder
│   └── app_audio_pdm/             # Standalone PDM digital microphone demo
├── shared/                        # Modular Drivers & Common Libraries
│   ├── drivers/                   # Hardware abstraction drivers
│   │   ├── ble/                   # BLE custom GATT services & connection manager
│   │   ├── imu/                   # LSM6DS3TR-C 6-axis IMU driver
│   │   ├── microphone/            # MSM261D3526H PDM DMIC driver
│   │   ├── rgb_led/               # Active-low RGB LED controller
│   │   └── ...                    # Battery, I2C, SPI, UART, USB CDC drivers
│   ├── lib/                       # DSP & Math algorithms
│   │   └── ekf/                   # Extended Kalman Filter implementation
│   └── include/                   # Shared C headers accessible across apps
├── Toolbox/                       # PC Software & Telemetry Tools
│   └── IMU3DBox/                  # Python 3D Visualizer & Audio Dashboard
│       ├── imu_3d_viewer.py       # Pygame 3D box, charts & sounddevice audio
│       └── USER.md                # Client setup & technical protocol reference
├── boards/                        # Board Support Package overlays
└── CMakeLists.txt                 # Top-level workspace build system
```

---

## 💻 Sample Applications

### 1. `app_ble_sensor` (Primary Application)
Combines 30Hz EKF 3D attitude estimation with 16kHz IMA ADPCM compressed PDM microphone audio streaming over Bluetooth LE.
- **Service UUID**: `12345678-1234-5678-1234-56789abcdef0`
- **IMU Characteristic**: `12345678-1234-5678-1234-56789abcdef1` (36-byte IEEE-754 float payload)
- **Audio Characteristic**: `12345678-1234-5678-1234-56789abcdef2` (128-byte ADPCM payload)

### 2. `Toolbox/IMU3DBox` (Python Client)
Interactive GUI application built with Pygame and Bleak that connects to `app_ble_sensor`.
- Perspective 3D cuboid model rendering.
- Real-time $32\times$ digital gain oscilloscope waveform chart.
- Low-latency PC speaker audio output via `sounddevice`.
- Interactive gyro calibration and axis remapping controls.

---

## ⚙️ Dependencies & Prerequisites

### Board & Build Environment
- **Nordic nRF Connect SDK (NCS)** v2.x or v3.2+ (includes Zephyr RTOS, CMake, and `west`).
- **Zephyr SDK Toolchain**: `arm-zephyr-eabi-gcc`.

### Python Visualizer Dependencies
Install the required packages using Python 3.10+:
```bash
pip install pygame bleak sounddevice numpy
```

---

## 🔨 Build & Flashing Instructions

### Step 1: Compile Firmware
Set up your nRF Connect environment and run `west build`:

```powershell
# Build main dual-stream BLE sensor app
west build -b xiao_ble/nrf52840/sense apps/app_ble_sensor --pristine
```

### Step 2: Flash to Board (UF2 Bootloader)
1. Connect the Seeed Studio XIAO nRF52840 Sense board to your PC using a USB-C cable.
2. **Double-click the reset button** on the board. The onboard LED will pulse green, and the board will mount as a USB mass storage volume named `XIAO-SENSE` (`E:\`).
3. Copy the compiled `.uf2` binary to the drive:
   ```powershell
   Copy-Item build/app_ble_sensor/zephyr/zephyr.uf2 -Destination E:\ -Force
   ```
4. The board will automatically flash itself, reboot into Zephyr RTOS, and start BLE advertising.

### Step 3: Run Python Visualizer
```bash
python Toolbox/IMU3DBox/imu_3d_viewer.py
```

---

## 🤖 AI Development Acknowledgment

This platform's firmware drivers, Extended Kalman Filter (EKF) quaternion mathematics, Zephyr PDM audio pipeline, BLE GATT services, and Python visualization tools were designed, optimized, and debugged with pair programming assistance from **Antigravity AI (Google DeepMind)**.

---

## 📄 License & Disclaimer

### Free to Use
This project is open-source and **completely free for use**, modification, distribution, and implementation in personal, educational, research, or commercial projects under the **MIT License**.

### "As-Is" Disclaimer & Responsibility
> **DISCLAIMER**: THIS SOFTWARE AND HARDWARE PLATFORM IS PROVIDED **"AS IS"**, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND NONINFRINGEMENT.
>
> IN NO EVENT SHALL THE AUTHORS, DEVELOPERS, OR CONTRIBUTORS BE LIABLE FOR ANY CLAIM, DAMAGES, OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT, OR OTHERWISE, ARISING FROM, OUT OF, OR IN CONNECTION WITH THE SOFTWARE, FIRMWARE, HARDWARE DEPLOYMENT, OR THE USE OR OTHER DEALINGS IN THIS PLATFORM.
>
> **THE ENTIRE RISK, RESPONSIBILITY, AND LIABILITY REGARDING THE SELECTION, DEPLOYMENT, OPERATION, AND USE OF THIS PLATFORM RESTS SOLELY WITH THE USER / OWNER.**
