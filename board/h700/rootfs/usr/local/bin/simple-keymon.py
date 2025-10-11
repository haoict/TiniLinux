#!/usr/bin/env python3

import evdev
import asyncio
import subprocess
import time

joypadInput = evdev.InputDevice("/dev/input/event2")
volumeInput = evdev.InputDevice("/dev/input/event1")
powerKeyInput = evdev.InputDevice("/dev/input/event0")

brightness_path = "/sys/devices/platform/backlight/backlight/backlight/brightness"
max_brightness = int(open("/sys/devices/platform/backlight/backlight/backlight/max_brightness", "r").read())


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

    fn = 708
    select = 314
    start = 315

def runcmd(cmd, *args, **kw):
    print(f">>> {cmd}")
    subprocess.run(cmd, *args, **kw)

def brightness(direction):
    with open(brightness_path, "r+") as f:
        cur = int(f.read())
        adj = 16
        cur = max(1, min(cur + adj * direction, max_brightness))
        f.seek(0, 0)
        f.write(f"{cur}")

def volume(direction):
    result = subprocess.run("amixer get -c 1 Master | awk -F'[][]' '/Left:/ { print $2 }' | sed 's/%//'", shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    print(result)
    if result.returncode == 0:
        cur = int(result.stdout)
        adj = 10
        cur = max(0, min(cur + adj * direction, 100))
        re = subprocess.run(f"amixer set -c 1 Master {cur}%", shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        print(re)

async def handle_event(device):
    # event.code is the button number
    # event.value is 1 for press, 0 for release
    # event.type is 1 for button, 3 for axis
    async for event in device.async_read_loop():
        if device.name == "retrogame_joypad":
            keys = joypadInput.active_keys()

            if Joypad.select in keys:
                if Joypad.b in keys:
                    if event.code == Joypad.start and event.value == 1:
                        #runcmd("if pidof simple-launcher > /dev/null; then killall simple-launcher; else cd /usr/local/bin && (LD_LIBRARY_PATH=/usr/local/lib /usr/local/bin/simple-launcher &); fi \n", shell=True)
                        runcmd("systemctl restart simple-init\n", shell=True)
                if event.code == Joypad.start and event.value == 1:
                    runcmd("killall retroarch; killall pico8_64; killall 351Files; true\n", shell=True)
                if event.code == Joypad.up and event.value == 1:
                    brightness(1)
                if event.code == Joypad.down and event.value == 1:
                    brightness(-1)
        elif device.name == "gpio-keys-vol":
            if event.code == 115 and event.value == 1:
                volume(1)
            if event.code == 114 and event.value == 1:
                volume(-1)
        elif device.name == "rk805 pwrkey":
            if event.code == 116 and event.value == 1:
                print("Power key pressed")
                # runcmd("systemctl suspend", shell=True)
        time.sleep(0.001)

def run():
    asyncio.ensure_future(handle_event(joypadInput))
    asyncio.ensure_future(handle_event(volumeInput))
    asyncio.ensure_future(handle_event(powerKeyInput))

    loop = asyncio.get_event_loop()
    loop.run_forever()

if __name__ == "__main__": # admire
    run()