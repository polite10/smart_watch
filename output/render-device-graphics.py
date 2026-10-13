"""Decode the captured physical LCD buffers using this board's actual GFXMMU LUT.

Run Verify-Graphics-Performance.ps1 or Verify-Refinements-Hardware.ps1 first.
These are board-rendered pixels;
the circular surround is added for presentation, not captured from a camera.
Requires Pillow. Raw dumps and device images are ignored by Git.
"""
from pathlib import Path
import re
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[1]
header = ROOT/'Drivers/BSP/STM32U5x9J-DK/stm32u5x9j_discovery_gfxmmu_lut.h'
lut = [int(v, 16) for v in re.findall(r'(0x[0-9A-Fa-f]+),?\s+/\* GFXMMU_LUT', header.read_text())][:960]
assert len(lut) == 960
mask = Image.new('L', (480, 480))
ImageDraw.Draw(mask).ellipse((0, 0, 479, 479), fill=255)
for dump in sorted((ROOT/'output').glob('device-*-framebuffer.bin')):
    raw = dump.read_bytes()
    assert len(raw) == 737280
    pixels = bytearray(480*480*4)
    for y in range(480):
        low, high = lut[2*y:2*y+2]
        first, last = (low >> 8) & 255, (low >> 16) & 255
        offset = high & 0x3fffff
        if offset & 0x200000:
            offset -= 0x400000
        for x in range(max(0, first*4), min(480, (last+1)*4)):
            pixels[(y*480+x)*4:(y*480+x+1)*4] = raw[offset+x*4:offset+x*4+4]
    im = Image.frombytes('RGBA', (480, 480), bytes(pixels), 'raw', 'BGRA').convert('RGB')
    panel = Image.new('RGB', (480, 480), '#171e2c')
    panel.paste(im, (0, 0), mask)
    panel.save(dump.with_name(dump.name.replace('-framebuffer.bin','.png')))
