"""Exercise the actual firmware's sensor-free IDA ViewModel in ARM emulation."""
import hashlib
import json
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tmp/simulator-libs'))
from unicorn import Uc, UC_ARCH_ARM, UC_MODE_THUMB, UC_MODE_MCLASS
from unicorn.arm_const import *
from elftools.elf.elffile import ELFFile

u = Uc(UC_ARCH_ARM, UC_MODE_THUMB | UC_MODE_MCLASS)
u.ctl_set_cpu_model(UC_CPU_ARM_CORTEX_M33)
for address, size in [(0x08000000, 0x400000), (0x20000000, 0x280000), (0x0fff0000, 0x1000)]:
    u.mem_map(address, size)
with (ROOT / 'Debug/Smartwatch.elf').open('rb') as source:
    elf = ELFFile(source)
    symbols = {s.name: s['st_value'] for s in elf.get_section_by_name('.symtab').iter_symbols() if s.name}
    for segment in elf.iter_segments():
        if segment['p_type'] == 'PT_LOAD':
            u.mem_write(segment['p_vaddr'], segment.data())
u.reg_write(UC_ARM_REG_C1_C0_2, 0xf00000)
u.reg_write(UC_ARM_REG_FPEXC, 0x40000000)
checks = []

def call(name, *args):
    for reg, value in zip([UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2, UC_ARM_REG_R3], args):
        u.reg_write(reg, value & 0xffffffff)
    u.reg_write(UC_ARM_REG_SP, 0x20270000)
    u.reg_write(UC_ARM_REG_LR, 0x0fff0001)
    u.emu_start(symbols[name] | 1, 0x0fff0000, count=10000000)
    if u.reg_read(UC_ARM_REG_PC) != 0x0fff0000:
        raise RuntimeError('Instruction limit: ' + name)
    return u.reg_read(UC_ARM_REG_R0)

def check(condition, label):
    checks.append((bool(condition), label))
    print(('PASS: ' if condition else 'FAIL: ') + label, flush=True)

call('watch_ida_vm_init')
address = call('watch_ida_vm_state')
def numeric():
    return struct.unpack('<6f', u.mem_read(address, 24))
def text(value):
    # The first 24 bytes are numeric; the remaining struct includes bounded trace and text buffers.
    return value.encode() + b'\0' in bytes(u.mem_read(address, 440))
def config():
    return struct.unpack('<I3H', u.mem_read(address + 24, 10))  # ETA, speed, range, sweep

check(abs(numeric()[5] - 420) < .01 and abs(numeric()[4] - 45) < .01, 'Initial target is 420 m northeast')
check(text('--:--') and text('HAZIR'), 'Unknown clock and simulation ready state')
call('watch_ida_vm_clock', 23, 59, 1)
check(text('23:59'), 'Valid RTC time is formatted')
call('watch_ida_vm_clock', 24, 0, 1)
check(text('--:--'), 'Invalid time never masquerades as a clock')
call('watch_ida_vm_clock', 12, 34, 1)
call('watch_ida_vm_step', 1000)
check(numeric()[0:2] == (0, 0) and config()[3] == 90, 'Sweep animates while ready; vehicle stays still')
before = numeric()
check(not call('watch_ida_vm_target', 153, 0) and not call('watch_ida_vm_target', 0, 0)
      and not call('watch_ida_vm_target', 2**31 - 1, 2**31 - 1) and numeric() == before,
      'Off-circle, neutral centre and extreme coordinates are rejected')
check(call('watch_ida_vm_target', 152, 0) and abs(numeric()[5] - 600) < .01 and abs(numeric()[4] - 90) < .01,
      'Selected range maps radar pixels to actual simulated metres')
call('watch_ida_vm_range')
check(config()[2] == 1200 and abs(numeric()[5] - 600) < .01, 'Zoom changes scale without changing world target')
call('watch_ida_vm_range'); call('watch_ida_vm_range')
check(config()[2] == 600, 'Range cycles through 1200, 200 and 600 m')
call('watch_ida_vm_toggle'); call('watch_ida_vm_step', 1000)
check(abs(numeric()[0] - 3.2924416) < .001 and numeric()[1] == 0 and numeric()[5] < 600 and text('AKTIF'),
      '6.4 knots moves 3.292 m each simulated second and reduces distance')
eta = config()[0]
call('watch_ida_vm_speed', 36); call('watch_ida_vm_step', 1000)
check(config()[1] == 100 and abs(numeric()[0] - 8.4368816) < .002 and config()[0] < eta,
      'Higher speed increases physical travel and reduces ETA')
call('watch_ida_vm_toggle'); before = numeric(); call('watch_ida_vm_step', 1000)
check(numeric() == before and text('DURAKLATILDI'), 'Pause freezes vehicle and target distance')
call('watch_ida_vm_toggle'); call('watch_ida_vm_target', 0, -152)
check(text('AKTIF') and abs(numeric()[4]) < .01, 'Retargeting a running task turns north without restarting it')
call('watch_ida_vm_pause'); before = numeric(); call('watch_ida_vm_step', 1000)
check(numeric() == before and text('DURAKLATILDI'), 'Leaving the screen can explicitly pause the task')
call('watch_ida_vm_speed', -2**31); call('watch_ida_vm_toggle'); before = numeric(); call('watch_ida_vm_step', 1000)
check(config()[1] == 0 and numeric() == before and text('--'), 'Zero speed freezes position and clears ETA')
call('watch_ida_vm_speed', 2**31 - 1)
check(config()[1] == 200, 'Speed clamps to 20 knots for extreme command values')
call('watch_ida_vm_reset')
check(numeric()[0:2] == (0, 0) and text('12:34') and text('-- m') and text('HAZIR'),
      'Reset clears task and trace, preserving RTC time')
call('watch_ida_vm_toggle'); call('watch_ida_vm_step', 1000)
check(text('HAZIR'), 'Cannot start without a target')
call('watch_ida_vm_target', -8, 0); call('watch_ida_vm_speed', 136); call('watch_ida_vm_toggle')
for _ in range(10):
    call('watch_ida_vm_step', 1000)
check(numeric()[5] == 0 and numeric()[0] == numeric()[2] and text('HEDEFE VARILDI') and config()[0] == 0,
      'Arrival snaps to target exactly, stops and has zero ETA')
before = numeric(); call('watch_ida_vm_toggle'); call('watch_ida_vm_step', 1000)
check(numeric() == before and text('HEDEFE VARILDI'), 'Arrival cannot accidentally restart old target')
call('watch_ida_vm_clear_target')
check(text('-- m') and text('HAZIR'), 'Clear target removes route and restores ready state')
call('watch_ida_vm_target', 152, 0); call('watch_ida_vm_toggle')
for _ in range(50):
    call('watch_ida_vm_step', 1000)
check(u.mem_read(address + 34, 1)[0] == 32, 'Long tasks keep trace bounded at 32 points')
before = numeric(); call('watch_ida_vm_step', 0)
check(numeric() == before, 'Zero elapsed time cannot move the vehicle')
call('watch_ida_vm_init'); call('watch_ida_vm_toggle'); call('watch_ida_vm_step', 600000)
check(abs(numeric()[5] - (420 - 3.2924416)) < .01, 'Large time jumps are capped instead of teleporting')
report = {'passed': sum(v for v, _ in checks), 'failed': sum(not v for v, _ in checks), 'checks': checks,
          'elf_sha256': hashlib.sha256((ROOT / 'Debug/Smartwatch.elf').read_bytes()).hexdigest(),
          'firmware_sha256': hashlib.sha256((ROOT / 'Firmware/Smartwatch.bin').read_bytes()).hexdigest(),
          'scope': 'Actual ARM IDA ViewModel, no LVGL or physical sensors initialized.'}
(ROOT / 'output/verify-ida.json').write_text(json.dumps(report, indent=2) + '\n')
sys.exit(1 if report['failed'] else 0)
