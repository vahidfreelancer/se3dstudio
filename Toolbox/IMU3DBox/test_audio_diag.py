import asyncio
import struct
import time
from bleak import BleakScanner, BleakClient

SERVICE_UUID = "12345678-1234-5678-1234-56789abcdef0"
IMU_CHAR_UUID = "12345678-1234-5678-1234-56789abcdef1"
AUDIO_CHAR_UUID = "12345678-1234-5678-1234-56789abcdef2"

audio_count = 0
imu_count = 0

def imu_callback(sender, data):
    global imu_count
    imu_count += 1

def audio_callback(sender, data):
    global audio_count
    audio_count += 1
    samples_count = len(data) // 2
    if samples_count > 0:
        pcm = struct.unpack(f'<{samples_count}h', data)
        min_v = min(pcm)
        max_v = max(pcm)
        avg_v = sum(abs(v) for v in pcm) / samples_count
        print(f"[{audio_count}] Audio Packet ({len(data)} bytes, {samples_count} samples): Min={min_v}, Max={max_v}, AvgAbs={avg_v:.1f}")

async def main():
    print("Scanning for se3dstudio_imu_01...")
    device_dict = await BleakScanner.discover(return_adv=True, timeout=3.0)
    target = None
    for addr, (d, adv) in device_dict.items():
        name = d.name or adv.local_name or ""
        if name.startswith("se3dstudio_imu_") or SERVICE_UUID in adv.service_uuids:
            target = d
            print(f"Found target: {name} [{addr}]")
            break
            
    if not target:
        print("Device not found!")
        return

    print(f"Connecting to {target.address}...")
    async with BleakClient(target.address) as client:
        print("Connected!")
        await client.start_notify(IMU_CHAR_UUID, imu_callback)
        print("Subscribed to IMU notifications.")
        
        try:
            await client.start_notify(AUDIO_CHAR_UUID, audio_callback)
            print("Subscribed to AUDIO notifications!")
        except Exception as e:
            print(f"FAILED to subscribe to AUDIO notifications: {e}")
            
        print("Listening for 10 seconds...")
        for i in range(10):
            await asyncio.sleep(1.0)
            print(f"t={i+1}s: IMU packets={imu_count}, Audio packets={audio_count}")

if __name__ == "__main__":
    asyncio.run(main())
