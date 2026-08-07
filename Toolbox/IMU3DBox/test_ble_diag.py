import asyncio
import struct
from bleak import BleakScanner, BleakClient

async def test():
    print("Scanning for BLE IMU...")
    devices = await BleakScanner.discover(timeout=5.0)
    target = None
    for d in devices:
        name = d.name or ""
        print(f"Device: {name} [{d.address}]")
        if name.startswith("se3dstudio_imu") or "XIAO" in name:
            target = d
            break
            
    if not target:
        print("Target device not found!")
        return

    print(f"Connecting to {target.name} [{target.address}]...")
    async with BleakClient(target.address) as client:
        print("Connected!")
        print("\n--- Discovered Services and Characteristics ---")
        for service in client.services:
            print(f"Service: {service.uuid}")
            for char in service.characteristics:
                print(f"  Characteristic: {char.uuid} | Properties: {char.properties}")
                
        # Try to find characteristic that supports notify
        notify_char = None
        for service in client.services:
            for char in service.characteristics:
                if "notify" in char.properties:
                    notify_char = char.uuid
                    print(f"\nFound Notify Characteristic: {notify_char}")

        if notify_char:
            count = 0
            def callback(sender, data):
                nonlocal count
                count += 1
                print(f"[{count}] Received Notification ({len(data)} bytes):")
                if len(data) == 36:
                    floats = struct.unpack('<9f', data)
                    print(f"   Roll={floats[0]:.2f}, Pitch={floats[1]:.2f}, Yaw={floats[2]:.2f}")
                    print(f"   Accel=({floats[3]:.2f}, {floats[4]:.2f}, {floats[5]:.2f})")
                    print(f"   Gyro=({floats[6]:.2f}, {floats[7]:.2f}, {floats[8]:.2f})")
                else:
                    print(f"   Raw bytes: {data.hex()}")

            print(f"Subscribing to {notify_char}...")
            await client.start_notify(notify_char, callback)
            print("Listening for 10 seconds...")
            await asyncio.sleep(10.0)

if __name__ == "__main__":
    asyncio.run(test())
