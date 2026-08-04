# GEMINI.md - Context & Agent Execution Guidelines

## 1. Project Overview & Target Architecture
- **Core Hardware:** Seeed Studio XIAO nRF52840 Sense Plus Core (nRF52840 ARM Cortex-M4F @ 64MHz, 1MB Flash, 256KB RAM).
- **Onboard Peripherals:** LSM6DS3TR-C (6-axis IMU), PDM Digital Microphone, Flash storage, Battery Management IC, BLE Antenna.
- **Framework:** Zephyr RTOS via Nordic nRF Connect SDK (NCS) / West build system.
- **Architecture Model:** Monorepo containing shared drivers/libraries and isolated applications under `apps/`.

---

## 2. Directory Structure Principles
```text
├── apps/
│   ├── app_template/     # Boilerplate application environment
│   ├── app_ble_sensor/   # Isolated Application 1
│   └── app_audio_pdm/    # Isolated Application 2
├── shared/
│   ├── drivers/          # Wrapped board-specific drivers (IMU, PDM)
│   ├── lib/              # Business logic, DSP, and helper utilities
│   └── include/          # Shared headers accessible by all apps
├── boards/               # Board support package (devicetree/Kconfig overlays)
├── CMakeLists.txt        # Top-level workspace build definition
└── GEMINI.md             # Antigravity Context & Execution Rules