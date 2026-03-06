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

gpioKeys = find_device_by_name("gpio-keys")
devices = [gpioKeys]

def runcmd(cmd, *args, **kw):
    print(f">>> {cmd}")
    subprocess.run(cmd, *args, **kw)

async def handle_event(device):
    # event.code is the button number
    # event.value is 1 for press, 0 for release
    # event.type is 1 for button, 3 for axis
    async for event in device.async_read_loop():
        if device.name == "gpio-keys":
            keys = gpioKeys.active_keys()

            if 1 in keys:  # esckey (key 1)
                if 44 in keys:  # x (key 2)
                    runcmd("killall retroarch; killall pico8_64; killall commander; killall simple-terminal; killall htop; killall install-nothing-linux-aarch64; true\n", shell=True)
                if 45 in keys:  # z key (key 3)
                    runcmd("""systemctl restart simple-init; true\n""", shell=True)
        time.sleep(0.001)

def run():
    loop = asyncio.new_event_loop()
    asyncio.set_event_loop(loop)
    
    loop.create_task(handle_event(gpioKeys))
    loop.run_forever()

if __name__ == "__main__": # admire
    run()