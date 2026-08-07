import asyncio
import struct
import sys
from bleak import BleakScanner, BleakClient

# Custom BLE Service and Characteristic UUIDs matching the firmware
SERVICE_UUID = "12345678-1234-5678-1234-56789abcdef0"
CHARACTERISTIC_UUID = "12345678-1234-5678-1234-56789abcdef1"

def make_ascii_bar(val, min_val, max_val, width=40):
    """Generates an ASCII bar visualization of a value within a range."""
    # Clamp value
    val = max(min_val, min(max_val, val))
    
    # Calculate percentage
    pct = (val - min_val) / (max_val - min_val)
    pos = int(pct * width)
    
    # Create the bar
    bar = ["-"] * (width + 1)
    center = width // 2
    bar[center] = "|"
    bar[pos] = "O"
    return "".join(bar)

async def main():
    print("=========================================================")
    print("      Wireless EKF-Fused IMU BLE Receiver Client")
    print("=========================================================")
    print("Scanning for BLE devices (5 seconds)...")
    
    device_dict = await BleakScanner.discover(return_adv=True, timeout=5.0)
    
    imu_device = None
    print("\nDiscovered Devices:")
    for addr, (d, adv) in device_dict.items():
        name = d.name or adv.local_name or "Unknown"
        services = adv.service_uuids
        print(f" - [{addr}] Name: {name}, Services: {services}")
        
        # Match by name prefix
        if name.startswith("se3dstudio_imu_"):
            imu_device = d
            print(f" --> Matched by name prefix: {name}!")
            break
            
        # Match by Service UUID
        elif SERVICE_UUID in services:
            imu_device = d
            print(f" --> Matched by Service UUID: {SERVICE_UUID}!")
            break

    if not imu_device:
        print("\nBLE IMU Sensor not found in the scanned list.")
        print("Please verify:")
        print(" 1. The board is powered on.")
        print(" 2. The RGB LED is blinking Red (indicating advertising).")
        return

    print(f"\nTarget BLE IMU selected: {imu_device.name or 'Unknown'} [{imu_device.address}]")
    print("Connecting...")

    # Clear terminal before displaying data
    print("\033[H\033[J", end="")

    def notification_handler(sender, data):
        if len(data) == 12:
            # Unpack 3 little-endian floats (Roll, Pitch, Yaw)
            roll, pitch, yaw = struct.unpack('<3f', data)
            
            # Create ASCII visualization bars
            roll_bar = make_ascii_bar(roll, -180.0, 180.0, width=30)
            pitch_bar = make_ascii_bar(pitch, -90.0, 90.0, width=30)
            yaw_bar = make_ascii_bar(yaw, -180.0, 180.0, width=30)
            
            # Print the values in-place in the console
            sys.stdout.write("\033[H") # Move cursor to top left
            sys.stdout.write("=========================================================\n")
            sys.stdout.write("      Wireless EKF-Fused IMU BLE Data Stream (30Hz)\n")
            sys.stdout.write("=========================================================\n\n")
            sys.stdout.write(f" Device Name: {imu_device.name or 'Unknown'} [{imu_device.address}]\n\n")
            sys.stdout.write(f" Roll  (X): {roll:6.1f}°  [-180 to  180]  {roll_bar}\n")
            sys.stdout.write(f" Pitch (Y): {pitch:6.1f}°  [ -90 to   90]  {pitch_bar}\n")
            sys.stdout.write(f" Yaw   (Z): {yaw:6.1f}°  [-180 to  180]  {yaw_bar}\n\n")
            sys.stdout.write("=========================================================\n")
            sys.stdout.write(" Press Ctrl+C to disconnect and exit.\n")
            sys.stdout.flush()
        else:
            print(f"Warning: Received unexpected packet size of {len(data)} bytes.")

    try:
        async with BleakClient(imu_device.address) as client:
            print("Connected successfully!")
            print("Subscribing to IMU notifications...")
            await client.start_notify(CHARACTERISTIC_UUID, notification_handler)
            
            # Loop forever until interrupted
            while True:
                await asyncio.sleep(0.1)
                
    except KeyboardInterrupt:
        print("\n\nDisconnecting...")
    except Exception as e:
        print(f"\nAn error occurred: {e}")
    finally:
        print("Connection closed.")

if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        pass
