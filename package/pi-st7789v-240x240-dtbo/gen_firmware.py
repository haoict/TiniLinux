#!/usr/bin/env python3


# Gamma tuning knobs - edit these
GAMMA_POS = [0xD0, 0x08, 0x0F, 0x09, 0x08, 0x05, 0x2C, 0x43, 0x42, 0x07, 0x10, 0x0E, 0x1C, 0x20] 
# alt:      [0xD0, 0x04, 0x0D, 0x11, 0x13, 0x2B, 0x3F, 0x54, 0x4C, 0x18, 0x0D, 0x0B, 0x1F, 0x23]
GAMMA_NEG = [0xD0, 0x08, 0x0F, 0x09, 0x08, 0x05, 0x2C, 0x43, 0x42, 0x07, 0x10, 0x0E, 0x1C, 0x20]
# alt:      [0xD0, 0x04, 0x0C, 0x11, 0x13, 0x2C, 0x3F, 0x44, 0x51, 0x2F, 0x1F, 0x1F, 0x20, 0x23]
#            V0    VS1   VS2   VS4   VS6   VS13  VS20  VS27  VS36  VS43  VS50  VS52  VS57  V63
#            |<-- shadows -->|           |<-- midtones -->|           |<-- highlights -->|
# To fix "text too bright": lower VS20 (idx 6) and VS27 (idx 7) on GAMMA_POS
# To fix "text too dim":    raise VS20 (idx 6) and VS27 (idx 7) on GAMMA_POS
# Move in steps of 0x04-0x08 and regenerate/reflash each time



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

add_cmd(0x01);                                               add_delay(150)  # Software reset
add_cmd(0x11);                                               add_delay(120)  # Sleep out (120ms min per datasheet)

#add_cmd(0x36, 0x00)                                                         # Rotate: 0°   normal
add_cmd(0x36, 0x60)                                                          # Rotate 90°
#add_cmd(0x36, 0xC0)                                                         # Rotate: 180°
#add_cmd(0x36, 0xA0)                                                         # Rotate: 270°

add_cmd(0x3A, 0x55)                                                          # Pixel format RGB565
add_cmd(0xB2, 0x0C, 0x0C, 0x00, 0x33, 0x33)                                  # Porch control
add_cmd(0xB7, 0x35)                                                          # Gate control
add_cmd(0xBB, 0x20)                                                          # VCOM 0.9V (was 0x19 = 0.725V)
add_cmd(0xC0, 0x2C)                                                          # LCM control
add_cmd(0xC2, 0x01)                                                          # VDV/VRH enable
add_cmd(0xC3, 0x13)                                                          # VRH 4.7V (was 0x12 = 4.6V)
add_cmd(0xC4, 0x20)                                                          # VDV set
add_cmd(0xC6, 0x0F)                                                          # Frame rate 60Hz
add_cmd(0xD0, 0xA4, 0xA1)                                                    # Power control

# Gamma correction - tuned for Waveshare IPS panel, comment out these 2 line if no need gamma
# add_cmd(0xE0, *GAMMA_POS)
# add_cmd(0xE1, *GAMMA_NEG)

add_cmd(0x21)                                                                # Inversion on
add_cmd(0x29);                                               add_delay(100)  # Display on

with open('panel-mipi-dbi-spi.bin', 'wb') as f:
    f.write(buf)

print(f"Written {len(buf)} bytes")
print(f"Magic: {buf[:8]!r}")
print(f"GAMMA_POS: {[hex(x) for x in GAMMA_POS]}")
print(f"GAMMA_NEG: {[hex(x) for x in GAMMA_NEG]}")
