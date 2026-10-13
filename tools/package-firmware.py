"""Export the linked Smartwatch application as a DeviceBoot v1 .dfw package."""
import hashlib
import json
from pathlib import Path
import struct
import zipfile
import zlib

root = Path(__file__).resolve().parents[1]
data = (root / 'Firmware/Smartwatch.bin').read_bytes()
base, limit = 0x0800A000, 0x083FC000
if not 0x410 <= len(data) <= limit - base:
    raise SystemExit('Firmware exceeds the DeviceBoot application slot')
sp, pc = struct.unpack_from('<II', data)
magic, app_id, version, protocol = struct.unpack_from('<4I', data, 0x400)
if (magic, app_id, protocol) != (0x44415631, 1, 1) or not base <= (pc & ~1) < base + len(data) or not pc & 1:
    raise SystemExit('Application was not linked for DeviceBoot')
if not 0x20000000 <= sp <= 0x2026FFF0 or sp & 7:
    raise SystemExit('Invalid stack / boot mailbox overlap')
manifest = dict(schema=1, board='STM32U5A9J-DK', protocol=1, base=base, id=1,
                name='Smartwatch', version=version, size=len(data),
                sha256=hashlib.sha256(data).hexdigest(), crc32=zlib.crc32(data))
with zipfile.ZipFile(root / 'Firmware/Smartwatch.dfw', 'w', zipfile.ZIP_DEFLATED) as archive:
    archive.writestr('manifest.json', json.dumps(manifest, indent=2))
    archive.writestr('firmware.bin', data)
print(f"Smartwatch.dfw: {len(data)} application bytes; SHA-256 {manifest['sha256']}")
