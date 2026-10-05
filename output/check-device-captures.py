"""Render real LTDC virtual framebuffer captures and compare preserved settings."""
from pathlib import Path
import argparse, hashlib, json, re, struct, zlib
from PIL import Image, ImageDraw

root = Path(__file__).resolve().parents[1]
out = root / 'output'
parser=argparse.ArgumentParser()
parser.add_argument('--backup',default='Backup/before-curved-ui-flash.bin')
args=parser.parse_args()
names = ['faces', 'neon', 'classic', 'orbit', 'alarms', 'menu', 'calculator', 'settings', 'keyboard', 'calendar', 'water']
sheet = Image.new('RGB', (1440, 1920), '#171e2c')
mask = Image.new('L', (480, 480))
ImageDraw.Draw(mask).ellipse((0, 0, 479, 479), fill=255)
lut_source = (root / 'Drivers/BSP/STM32U5x9J-DK/stm32u5x9j_discovery_gfxmmu_lut.h').read_text()
lut = [int(v, 16) for v in re.findall(r'(0x[0-9A-Fa-f]+),?\s+/\* GFXMMU_LUT', lut_source)]
assert len(lut) >= 960
lut = lut[:960]
for i, name in enumerate(names):
    raw = (out / f'device-{name}-framebuffer.bin').read_bytes()
    pixels = bytearray(480*480*4)
    for y in range(480):
        low, high = lut[2*y:2*y+2]
        first, last = (low >> 8) & 255, (low >> 16) & 255
        offset = high & 0x3fffff
        if offset & 0x200000:
            offset -= 0x400000
        for x in range(max(0, first*4), min(480, (last+1)*4)):
            src = offset + x*4
            dst = (y*480+x)*4
            pixels[dst:dst+4] = raw[src:src+4]
    assert any(pixels), 'Physical framebuffer is empty'
    im = Image.frombytes('RGBA', (480, 480), bytes(pixels), 'raw', 'BGRA').convert('RGB')
    panel = Image.new('RGB', (480, 480), '#171e2c')
    panel.paste(im, (0, 0), mask)
    panel.save(out / f'device-{name}.png')
    sheet.paste(panel, ((i % 3) * 480, (i // 3) * 480))
sheet.save(out / 'device-ui-preview.png')

backup = (root / args.backup).read_bytes()
records = []
for offset in range(0x3fc000, 0x400000, 1024):
    magic, seq, crc, size = struct.unpack_from('<4I', backup, offset)
    if magic != 0x53574132 or size != 916:
        continue
    payload = backup[offset+16:offset+16+size]
    if zlib.crc32(payload) == crc:
        records.append((seq, payload))
assert records, 'No valid settings journal in pre-upload backup'
before = max(records, key=lambda item: item[0])[1]
after = (out / 'settings-after-curved-boot.bin').read_bytes()
checks = {
    'data_size_916': len(after) == 916,
    'notes_preserved': before[28:796] == after[28:796],
    'alarms_preserved': all(before[4+i*8:8+i*8] == after[4+i*8:8+i*8] for i in range(3)),
    'water_history_preserved': before[796:908] == after[796:908],
    'goal_reminder_format_theme_preserved': before[908:914] == after[908:914],
}
resident = (out / 'settings-flash-after.bin').read_bytes()
resident_records = []
for offset in range(0, 16384, 1024):
    magic, seq, crc, size = struct.unpack_from('<4I', resident, offset)
    if magic == 0x53574132 and size == 916 and zlib.crc32(resident[offset+16:offset+16+size]) == crc:
        resident_records.append((seq, resident[offset+16:offset+16+size]))
resident_seq, resident_payload = max(resident_records, key=lambda item: item[0])
checks['current_face_loaded_from_valid_flash_record'] = resident_payload[914:916] == after[914:916]
checks['uploaded_firmware_matches_build'] = (out / 'device-firmware.bin').read_bytes() == (root / 'Firmware/Smartwatch.bin').read_bytes()
log = (out / 'verify-curved-hardware.log').read_text()
times = {name: int(value) for name, value in re.findall(r'(\w+) FRAME: (\d+) ms', log)}
checks['hardware_checks_passed'] = 'HARDWARE FAILURES: 0' in log
report = {
    'checks': checks,
    'hardware_passed': len(re.findall(r'^PASS:', log, re.M)),
    'frame_ms': times,
    'pre_upload_settings_sequence': max(records, key=lambda item: item[0])[0],
    'current_settings_sequence': resident_seq,
    'face_changed_since_backup': before[914:916] != after[914:916],
    'elf_sha256': hashlib.sha256((root / 'Debug' / 'Smartwatch.elf').read_bytes()).hexdigest(),
    'firmware_sha256': hashlib.sha256((root / 'Firmware' / 'Smartwatch.bin').read_bytes()).hexdigest(),
    'scope': 'Actual STM32 framebuffer captures and injected touch samples via ST-LINK. Not panel photographs or physical finger tests. Times cover first flush through presentation and buffer synchronization, excluding the initial draw strip and gesture recognition delay.',
}
(out / 'verify-curved-hardware.json').write_text(json.dumps(report, indent=2))
print(json.dumps(report, indent=2))
assert all(checks.values()), 'Device validation failed'
