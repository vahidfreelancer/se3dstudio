import asyncio
import wave
import sys
from bleak import BleakScanner, BleakClient

# Custom BLE Audio UUIDs matching firmware
SERVICE_UUID = "19b10000-e8f2-537e-4f6c-d104768a1214"
CHARACTERISTIC_UUID = "19b10001-e8f2-537e-4f6c-d104768a1214"

# IMA ADPCM Lookup Tables
INDEX_TABLE = [
    -1, -1, -1, -1, 2, 4, 6, 8,
    -1, -1, -1, -1, 2, 4, 6, 8
]

STEPSIZE_TABLE = [
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

class AdpcmDecoder:
    def __init__(self):
        self.valprev = 0
        self.index = 0

    def decode_sample(self, code):
        step = STEPSIZE_TABLE[self.index]
        
        diff = step >> 3
        if code & 4: diff += step
        if code & 2: diff += step >> 1
        if code & 1: diff += step >> 2

        if code & 8:
            self.valprev -= diff
        else:
            self.valprev += diff

        # Clamp prediction
        if self.valprev > 32767:
            self.valprev = 32767
        elif self.valprev < -32768:
            self.valprev = -32768

        # Update step index
        self.index += INDEX_TABLE[code & 7]
        if self.index < 0:
            self.index = 0
        elif self.index > 88:
            self.index = 88

        return self.valprev

    def decode_block(self, encoded):
        pcm = bytearray()
        for b in encoded:
            code1 = b & 0x0F
            code2 = (b >> 4) & 0x0F
            
            s1 = self.decode_sample(code1)
            s2 = self.decode_sample(code2)
            
            pcm.extend(s1.to_bytes(2, byteorder='little', signed=True))
            pcm.extend(s2.to_bytes(2, byteorder='little', signed=True))
        return pcm

async def main():
    print("Scanning for BLE devices (5 seconds)...")
    device_dict = await BleakScanner.discover(return_adv=True, timeout=5.0)
    
    mic_device = None
    print("\nDiscovered Devices:")
    for addr, (d, adv) in device_dict.items():
        name = d.name or adv.local_name or "Unknown"
        services = adv.service_uuids
        print(f" - [{addr}] Name: {name}, Services: {services}")
        
        # Match by name prefix
        if name.startswith("se3dstudio_mic_"):
            mic_device = d
            print(f" --> Matched by name prefix: {name}!")
            
        # Match by Service UUID
        elif SERVICE_UUID in services:
            mic_device = d
            print(f" --> Matched by Service UUID: {SERVICE_UUID}!")

    if not mic_device:
        print("\nBLE Microphone not found in the scanned list.")
        print("Please verify:")
        print(" 1. The board is powered on.")
        print(" 2. The RGB LED is blinking Red (indicating advertising).")
        return

    print(f"\nTarget BLE Microphone selected: {mic_device.name or 'Unknown'} [{mic_device.address}]")
    print("Connecting...")

    decoder = AdpcmDecoder()
    wav_file = wave.open("recording.wav", "wb")
    wav_file.setnchannels(1)      # Mono
    wav_file.setsampwidth(2)     # 16-bit
    wav_file.setframerate(16000)  # 16 kHz

    total_bytes_received = 0

    def notification_handler(sender, data):
        nonlocal total_bytes_received
        pcm_data = decoder.decode_block(data)
        wav_file.writeframes(pcm_data)
        total_bytes_received += len(data)
        print(f"\rStreaming audio... Received {total_bytes_received} bytes", end="")

    async with BleakClient(mic_device.address) as client:
        print("Connected successfully!")
        print("Enabling audio notifications...")
        await client.start_notify(CHARACTERISTIC_UUID, notification_handler)
        print("Recording started. Save output to 'recording.wav'. Press Ctrl+C to stop.")

        try:
            while True:
                await asyncio.sleep(0.1)
        except KeyboardInterrupt:
            print("\nStopping...")
        finally:
            print("Disabling notifications...")
            await client.stop_notify(CHARACTERISTIC_UUID)
            wav_file.close()
            print("Wav file successfully saved to 'recording.wav'. Connection closed.")

if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        pass
