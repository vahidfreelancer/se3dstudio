#!/usr/bin/env bash

# Exit immediately if a command exits with a non-zero status
set -e

# Configuration
NCS_VERSION="v3.2.1"
INSTALL_DIR="$HOME/ncs"
NRFUTIL_DIR="$HOME/.nrfutil/bin"
PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo "========================================================================"
# Print starting banner
echo "Starting Seeed Studio nRF52840 monorepo setup on Ubuntu 24.04..."
echo "========================================================================"

# 1. Install Ubuntu System Dependencies
echo -e "\n[1/5] Installing system dependencies (requires sudo privileges)..."
sudo apt-get update
sudo apt-get install -y \
    cmake \
    ninja-build \
    gperf \
    ccache \
    dfu-util \
    device-tree-compiler \
    wget \
    python3-pip \
    python3-venv \
    python3-setuptools \
    python3-tk \
    python3-wheel \
    xz-utils \
    file \
    make \
    gcc \
    g++ \
    libsdl2-dev \
    libmagic1 \
    libglib2.0-0

# 2. Download and Setup nrfutil
echo -e "\n[2/5] Setting up Nordic nrfutil CLI..."
mkdir -p "$NRFUTIL_DIR"
export PATH="$NRFUTIL_DIR:$PATH"

if [ ! -f "$NRFUTIL_DIR/nrfutil" ]; then
    echo "Downloading nrfutil tool..."
    wget -q https://developer.nordicsemi.com/.pc-tools/nrfutil/x64-linux/nrfutil -O "$NRFUTIL_DIR/nrfutil"
    chmod +x "$NRFUTIL_DIR/nrfutil"
else
    echo "nrfutil already installed."
fi

# 3. Install Nordic Toolchain and nRF Connect SDK (NCS)
echo -e "\n[3/5] Setting up nRF Connect SDK $NCS_VERSION and toolchain..."
# Register toolchain-manager if not already registered
"$NRFUTIL_DIR/nrfutil" install toolchain-manager

# Install the toolchain and SDK
mkdir -p "$INSTALL_DIR"
echo "Installing toolchain & SDK (this may take several minutes)..."
"$NRFUTIL_DIR/nrfutil" toolchain-manager install --ncs-version "$NCS_VERSION" --dir "$INSTALL_DIR"

# 4. Initialize and Configure Python Virtual Environment
echo -e "\n[4/5] Setting up host python dependencies..."
if [ -d "$PROJECT_DIR/venv" ]; then
    echo "Python virtual environment already exists."
else
    echo "Creating virtual environment..."
    python3 -m venv "$PROJECT_DIR/venv"
fi

# Activate virtual environment
source "$PROJECT_DIR/venv/bin/activate"
pip install --upgrade pip
pip install bleak wave

# 5. Compile Applications
echo -e "\n[5/5] Compiling applications..."

# Compile app_template
echo "Building app_template..."
"$NRFUTIL_DIR/nrfutil" toolchain-manager launch --ncs-version "$NCS_VERSION" --dir "$INSTALL_DIR" -- \
    west build -b xiao_ble/nrf52840/sense "$PROJECT_DIR/apps/app_template" --pristine

# Compile Ble-mic
echo "Building Ble-mic..."
"$NRFUTIL_DIR/nrfutil" toolchain-manager launch --ncs-version "$NCS_VERSION" --dir "$INSTALL_DIR" -- \
    west build -b xiao_ble/nrf52840/sense "$PROJECT_DIR/apps/Ble-mic" --pristine

# Compile app_ble_sensor
echo "Building app_ble_sensor..."
"$NRFUTIL_DIR/nrfutil" toolchain-manager launch --ncs-version "$NCS_VERSION" --dir "$INSTALL_DIR" -- \
    west build -b xiao_ble/nrf52840/sense "$PROJECT_DIR/apps/app_ble_sensor" --pristine

echo "========================================================================"
echo "Setup and compilation completed successfully!"
echo "Binaries are located at:"
echo " - Template App:   $INSTALL_DIR/$NCS_VERSION/build/app_template/zephyr/zephyr.uf2"
echo " - BLE Mic App:    $INSTALL_DIR/$NCS_VERSION/build/Ble-mic/zephyr/zephyr.uf2"
echo " - BLE Sensor App: $INSTALL_DIR/$NCS_VERSION/build/app_ble_sensor/zephyr/zephyr.uf2"
echo "========================================================================"
echo "To run the BLE IMU 3D Visualizer, activate your python virtual env and run:"
echo "  source $PROJECT_DIR/venv/bin/activate"
echo "  python $PROJECT_DIR/Toolbox/IMU3DBox/imu_3d_viewer.py"
echo "========================================================================"
