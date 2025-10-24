#!/usr/bin/env python3

import evdev
import asyncio
import subprocess
import time

def find_device_by_name(name):
    devices = [evdev.InputDevice(path) for path in evdev.list_devices()]
    for device in devices:
        if device.name == name:
            return device
    raise Exception(f"Input device with name '{name}' not found")

joypadInput = find_device_by_name("H700 Gamepad")
volumeInput = find_device_by_name("gpio-keys-volume")
powerKeyInput = find_device_by_name("axp20x-pek")
# rg35xxspLid = find_device_by_name("gpio-keys-lid")
devices = [joypadInput, volumeInput, powerKeyInput]

brightness_path = "/sys/devices/platform/backlight/backlight/backlight/brightness"
max_brightness = int(open("/sys/devices/platform/backlight/backlight/backlight/max_brightness", "r").read())

suspended = 0

class Joypad:
    l1 = 310
    r1 = 311
    l2 = 312
    r2 = 313

    up = 544
    down = 545
    left = 546
    right = 547

    x = 307
    y = 308
    a = 305
    b = 304

    fn = 316
    select = 314
    start = 315

def runcmd(cmd, *args, **kw):
    print(f">>> {cmd}")
    subprocess.run(cmd, *args, **kw)

def brightness(direction):
    with open(brightness_path, "r") as f:
        cur = int(f.read().strip())
    adj = max_brightness * 5 / 100 # 5% of max brightness
    cur = max(1, min(cur + adj * direction, max_brightness))
    
    with open(brightness_path, "w") as f:
        f.write(f"{int(cur)}\n")

def volume(direction):
    result = subprocess.run("amixer get -c 0 DAC | awk -F'[][]' '/Left:/ { print $2 }' | sed 's/%//'", shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    print(result)
    if result.returncode == 0:
        cur = int(result.stdout)
        adj = 10
        cur = max(0, min(cur + adj * direction, 100))
        re = subprocess.run(f"amixer set -c 0 DAC {cur}%", shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        print(re)

async def handle_event(device):
    # event.code is the button number
    # event.value is 1 for press, 0 for release
    # event.type is 1 for button, 3 for axis
    async for event in device.async_read_loop():
        if device.name == "H700 Gamepad":
            keys = joypadInput.active_keys()

            if Joypad.fn in keys:
                if Joypad.select in keys:
                    if event.code == Joypad.start and event.value == 1:
                        runcmd("systemctl restart simple-init\n", shell=True)
                if event.code == Joypad.start and event.value == 1:
                    runcmd("killall retroarch; killall pico8_64; killall 351Files; true\n", shell=True)
                if event.code == Joypad.up and event.value == 1:
                    brightness(1)
                if event.code == Joypad.down and event.value == 1:
                    brightness(-1)
        elif device.name == "gpio-keys-volume":
            if event.code == 115 and event.value == 1:
                volume(1)
            if event.code == 114 and event.value == 1:
                volume(-1)
        elif device.name == "axp20x-pek":
            if event.code == 116 and event.value == 1:
                print("Power key pressed")
            #     global suspended
            #     if suspended == 0:
            #         suspended = 1
            #         runcmd("systemctl suspend", shell=True)
            #     else:
            #         suspended = 0
        time.sleep(0.001)

def run():
    asyncio.ensure_future(handle_event(joypadInput))
    asyncio.ensure_future(handle_event(volumeInput))
    asyncio.ensure_future(handle_event(powerKeyInput))

    loop = asyncio.get_event_loop()
    loop.run_forever()

if __name__ == "__main__": # admire
    run()