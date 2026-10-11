"""Run output/test-arcade.c built as a standalone ARM ELF (no board needed)."""
from pathlib import Path
import sys
import json
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tmp/simulator-libs'))
from unicorn import Uc, UC_ARCH_ARM, UC_MODE_THUMB, UC_MODE_MCLASS, UC_HOOK_CODE
from unicorn.arm_const import *
from elftools.elf.elffile import ELFFile

ROOT = Path(__file__).resolve().parents[1]
u = Uc(UC_ARCH_ARM, UC_MODE_THUMB | UC_MODE_MCLASS)
u.ctl_set_cpu_model(UC_CPU_ARM_CORTEX_M33)
u.mem_map(0x08000000, 0x400000)
u.mem_map(0x20000000, 0x280000)
u.mem_map(0x0fff0000, 0x1000)
with (ROOT / 'tmp/test-arcade.elf').open('rb') as source:
    elf = ELFFile(source)
    symbols = {s.name: s['st_value'] for s in elf.get_section_by_name('.symtab').iter_symbols() if s.name}
    for segment in elf.iter_segments():
        if segment['p_type'] == 'PT_LOAD':
            u.mem_write(segment['p_vaddr'], segment.data())
u.reg_write(UC_ARM_REG_C1_C0_2, 0xf00000)
u.reg_write(UC_ARM_REG_FPEXC, 0x40000000)

def string(address):
    return bytes(u.mem_read(address, 512)).split(b'\0')[0].decode()

def assertion(uc, address, size, data):
    raise AssertionError(f'{string(u.reg_read(UC_ARM_REG_R0))}:{u.reg_read(UC_ARM_REG_R1)}: {string(u.reg_read(UC_ARM_REG_R3))}')

def puts(uc, address, size, data):
    print(string(u.reg_read(UC_ARM_REG_R0)), flush=True)
    u.reg_write(UC_ARM_REG_R0, 0)
    u.reg_write(UC_ARM_REG_PC, u.reg_read(UC_ARM_REG_LR))

for name, hook in [('__assert_func', assertion), ('puts', puts)]:
    address = symbols[name] & ~1
    u.hook_add(UC_HOOK_CODE, hook, begin=address, end=address)
u.reg_write(UC_ARM_REG_SP, 0x20270000)
u.reg_write(UC_ARM_REG_LR, 0x0fff0001)
u.emu_start(symbols['main'] | 1, 0x0fff0000, count=100000000)
assert u.reg_read(UC_ARM_REG_PC) == 0x0fff0000 and u.reg_read(UC_ARM_REG_R0) == 0
report = {'passed': True, 'scope': 'Standalone ARM C assertions: 512 boards, all four seams, repeated wraps, seam growth/body collision, tail vacancy, full board, games/records and dirty rows.'}
(ROOT / 'output/verify-arcade-model.json').write_text(json.dumps(report, indent=2) + '\n')
print('ARCADE_MODEL_OK', flush=True)
