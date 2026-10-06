"""Build CubeIDE source entries using its bundled ARM GCC on macOS.

Usage: python3 output/build-macos.py [--clean]
Windows keeps Build.ps1. Dependency files track headers for incremental builds.
"""
from pathlib import Path
import argparse
import concurrent.futures
import hashlib
import json
import shlex
import subprocess
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--clean', action='store_true')
args = parser.parse_args()
tools = sorted(Path('/Applications/STM32CubeIDE.app/Contents/Eclipse/plugins').glob(
    'com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.*/tools/bin'))
if not tools:
    raise SystemExit('CubeIDE bundled ARM GCC not found')
BIN = tools[-1]
OUT = ROOT / 'Debug'
OUT.mkdir(exist_ok=True)
config = ET.parse(ROOT / '.cproject').getroot().find('.//cconfiguration')
includes = ['Core/Inc', 'App', 'Middlewares', 'Middlewares/lvgl', 'Drivers/CMSIS/Include',
            'Drivers/CMSIS/Device/ST/STM32U5xx/Include', 'Drivers/STM32U5xx_HAL_Driver/Inc',
            'Drivers/BSP/STM32U5x9J-DK', 'Drivers/BSP/Components/Common']
flags = ['-mcpu=cortex-m33', '-mthumb', '-mfpu=fpv5-sp-d16', '-mfloat-abi=hard', '-O2', '-g3',
         '-ffunction-sections', '-fdata-sections', '-fshort-enums', '-Wall', '-Wno-format',
         '-DDEBUG', '-DUSE_HAL_DRIVER', '-DSTM32U5A9xx', '-DLV_CONF_INCLUDE_SIMPLE']
flags += [f'-I{ROOT / i}' for i in includes]
sources = []
for entry in config.findall('.//sourceEntries/entry'):
    folder = ROOT / entry.get('name')
    excluded = set(entry.get('excluding', '').split('|'))
    sources.extend(p for p in folder.rglob('*') if p.suffix in ('.c', '.s') and p.name not in excluded)
sources.sort()
signature = json.dumps({'compiler': str(BIN), 'flags': flags, 'app_wextra': True})
cache = OUT / 'macos-build-config.json'
rebuild = args.clean or not cache.exists() or cache.read_text() != signature

def compile_one(source):
    obj = OUT / (str(source.relative_to(ROOT)).replace('/', '_') + '.o')
    dep = obj.with_suffix('.d')
    warning_file = obj.with_suffix('.warnings')
    if not rebuild and obj.exists() and dep.exists() and warning_file.exists():
        dependencies = shlex.split(dep.read_text().replace('\\\n', ' ').partition(':')[2])
        if dependencies and all(Path(p).exists() and Path(p).stat().st_mtime <= obj.stat().st_mtime for p in dependencies):
            return obj, warning_file.read_text(), False
    language = ['-std=gnu11'] if source.suffix == '.c' else ['-x', 'assembler-with-cpp']
    extra = ['-Wextra'] if source.is_relative_to(ROOT / 'App') else []
    result = subprocess.run([str(BIN / 'arm-none-eabi-gcc'), *flags, *extra, *language,
                             '-MMD', '-MF', str(dep), '-c', str(source), '-o', str(obj)],
                            capture_output=True, text=True)
    if result.returncode:
        obj.unlink(missing_ok=True)
        raise RuntimeError(str(source.relative_to(ROOT)) + '\n' + result.stderr)
    warning_file.write_text(result.stderr)
    return obj, result.stderr, True

print(f'Checking {len(sources)} sources with CubeIDE ARM GCC…', flush=True)
with concurrent.futures.ThreadPoolExecutor(max_workers=8) as pool:
    try:
        results = list(pool.map(compile_one, sources))
    except Exception as error:
        raise SystemExit(str(error))
warnings = ''.join(r[1] for r in results)
if warnings:
    print(warnings)
(OUT / 'objects.rsp').write_text('\n'.join('"' + str(r[0]) + '"' for r in results))
elf = OUT / 'Smartwatch.elf'
command = [str(BIN / 'arm-none-eabi-gcc'), *flags, '@' + str(OUT / 'objects.rsp'),
           '-T' + str(ROOT / 'STM32U5A9NJHXQ_FLASH.ld'), '--specs=nano.specs', '--specs=nosys.specs',
           '-Wl,--gc-sections,-Map=' + str(OUT / 'Smartwatch.map'), '-Wl,-u,_printf_float',
           '-Wl,-u,_scanf_float', '-Wl,-z,noexecstack', '-Wl,--start-group', '-lc', '-lm',
           '-Wl,--end-group', '-o', str(elf)]
subprocess.run(command, check=True)
size = subprocess.run([str(BIN / 'arm-none-eabi-size'), str(elf)], check=True, capture_output=True, text=True).stdout
print(size, end='')
binary = ROOT / 'Firmware/Smartwatch.bin'
subprocess.run([str(BIN / 'arm-none-eabi-objcopy'), '-O', 'binary', str(elf), str(binary)], check=True)
cache.write_text(signature)
report = {'compiler': str(BIN / 'arm-none-eabi-gcc'), 'sources': len(sources),
          'compiled': sum(r[2] for r in results), 'warnings': warnings, 'size': size,
          'elf_sha256': hashlib.sha256(elf.read_bytes()).hexdigest(),
          'firmware_sha256': hashlib.sha256(binary.read_bytes()).hexdigest()}
(ROOT / 'output/build-arcade.json').write_text(json.dumps(report, indent=2) + '\n')
print('Build complete; Firmware/Smartwatch.bin matches this ELF.', flush=True)
