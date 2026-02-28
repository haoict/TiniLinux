#!/usr/bin/env python3
import struct

buf = bytearray()

# Magic: 'MIPI DBI' + 7 null bytes = 15 bytes
buf += b'MIPI DBI'
buf += b'\x00' * 7

# File format version (1 byte)
buf += b'\x01'

# Command format: [cmd_byte, num_params, param0, param1, ...]
# Sleep/delay: [0x00, 0x01, delay_ms]

def add_cmd(cmd, *params):
    buf.extend([cmd, len(params)] + list(params))

def add_delay(ms):
    buf.extend([0x00, 0x01, min(ms, 255)])

add_cmd(0x01);                                              add_delay(150)   # Software reset
add_cmd(0x11);                                              add_delay(255)   # Sleep out

#add_cmd(0x36, 0x00)   # rotate: 0°   normal
add_cmd(0x36, 0x60)   # rotate: 90°
#add_cmd(0x36, 0xC0)   # rotate: 180°
#add_cmd(0x36, 0xA0)   # rotate: 270°

add_cmd(0x3A, 0x55)                                                          # Pixel format RGB565
add_cmd(0xB2, 0x0C, 0x0C, 0x00, 0x33, 0x33)                                  # Porch control
add_cmd(0xB7, 0x35)                                                          # Gate control
add_cmd(0xBB, 0x19)                                                          # VCOM
add_cmd(0xC0, 0x2C)                                                          # LCM control
add_cmd(0xC2, 0x01)                                                          # VDV/VRH enable
add_cmd(0xC3, 0x12)                                                          # VRH set
add_cmd(0xC4, 0x20)                                                          # VDV set
add_cmd(0xC6, 0x0F)                                                          # Frame rate
add_cmd(0xD0, 0xA4, 0xA1)                                                    # Power control
add_cmd(0x21)                                                                # Inversion on
add_cmd(0x29);                                              add_delay(100)  # Display on

with open('panel-mipi-dbi-spi.bin', 'wb') as f:
    f.write(buf)

print(f"Written {len(buf)} bytes")
print(f"Magic: {buf[:8]!r}")