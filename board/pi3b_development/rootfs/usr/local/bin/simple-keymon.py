#!/usr/bin/env python3

import evdev
import asyncio
import subprocess
import time

brightness_path = "/sys/class/backlight/backlight/brightness"
max_brightness = int(open("/sys/class/backlight/backlight/max_brightness", "r").read())

# evdev key codes for standard keyboard
KEY_ESC = 1
KEY_X   = 45
KEY_Z   = 44
KEY_UP  = 103
KEY_DOWN = 108

def find_all_keyboards():
    """Find all keyboard-capable input devices (including gpio-keys)."""
    found = []
    for path in evdev.list_devices():
        try:
            device = evdev.InputDevice(path)
            caps = device.capabilities()
            if evdev.ecodes.EV_KEY in caps and KEY_ESC in caps[evdev.ecodes.EV_KEY]:
                print(f"Using input device: {device.name} ({device.path})")
                found.append(device)
        except Exception as e:
            print(f"Skipping {path}: {e}")
    if not found:
        raise Exception("No suitable keyboard/gpio device found")
    return found

def runcmd(cmd, *args, **kw):
    print(f">>> {cmd}")
    subprocess.run(cmd, *args, **kw)

def brightness(direction):
    with open(brightness_path, "r") as f:
        cur = int(f.read().strip())
    adj = 2 # 0-2-4-6-8
    cur = max(0, min(cur + adj * direction, max_brightness))
    runcmd(f"echo -e '\nSetting brightness: {cur}' > /dev/tty0", shell=True)
    with open(brightness_path, "w") as f:
        f.write(f"{int(cur)}\n")
    

async def handle_event(device):
    # event.value: 1 = press, 0 = release, 2 = repeat
    async for event in device.async_read_loop():
        if event.type == evdev.ecodes.EV_KEY and event.value == 1:  # key press only
            active = device.active_keys()

            if KEY_ESC in active:
                if KEY_X in active:
                    runcmd(
                        "killall retroarch; killall pico8_64; killall commander; "
                        "killall simple-terminal; killall htop; "
                        "killall install-nothing-linux-aarch64; killall fbdoom; killall -INT fbgif-linux-aarch64; true;",
                        shell=True
                    )
                elif KEY_Z in active:
                    runcmd("systemctl restart launcher; true", shell=True)
                elif KEY_UP in active:
                    brightness(1)
                elif KEY_DOWN in active:
                    brightness(-1)

        time.sleep(0.001)

def run():
    devices = find_all_keyboards()

    loop = asyncio.new_event_loop()
    asyncio.set_event_loop(loop)

    for device in devices:
        loop.create_task(handle_event(device))

    loop.run_forever()

if __name__ == "__main__":
    run()