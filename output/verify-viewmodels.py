"""Run compiled ARM Models/ViewModels without initializing LVGL or the board.

The fake flash and RTC ports exercise real command, draft and persistence flows.
Run after Build.ps1 and export the matching Firmware/Smartwatch.bin.
"""
import hashlib
import json
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tmp' / 'simulator-libs'))
from unicorn import Uc, UC_ARCH_ARM, UC_MODE_THUMB, UC_MODE_MCLASS, UC_HOOK_CODE
from unicorn.arm_const import *
from elftools.elf.elffile import ELFFile

u = Uc(UC_ARCH_ARM, UC_MODE_THUMB | UC_MODE_MCLASS)
u.ctl_set_cpu_model(UC_CPU_ARM_CORTEX_M33)
u.mem_map(0x08000000, 0x400000)
u.mem_map(0x20000000, 0x280000)
u.mem_map(0x21000000, 0x100000)
u.mem_map(0x0fff0000, 0x1000)
with (ROOT / 'Debug' / 'Smartwatch.elf').open('rb') as f:
    elf = ELFFile(f)
    symbols = {s.name: s['st_value'] for s in elf.get_section_by_name('.symtab').iter_symbols() if s.name}
    for seg in elf.iter_segments():
        if seg['p_type'] == 'PT_LOAD':
            u.mem_write(seg['p_vaddr'], seg.data())
u.reg_write(UC_ARM_REG_C1_C0_2, 0xf00000)
u.reg_write(UC_ARM_REG_FPEXC, 0x40000000)
regs = [UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2, UC_ARM_REG_R3]
persisted = None
storage_ok = True
clock_ok = True
clock_calls = []
checks = []

def ret(value=0):
    u.reg_write(UC_ARM_REG_R0, value)
    u.reg_write(UC_ARM_REG_PC, u.reg_read(UC_ARM_REG_LR))

def replace(name, callback):
    addr = symbols[name] & ~1
    u.hook_add(UC_HOOK_CODE, lambda uc, a, size, data: callback(), begin=addr, end=addr)

def load():
    if persisted is not None:
        u.mem_write(u.reg_read(UC_ARM_REG_R0), persisted)
    ret(persisted is not None)

def save():
    global persisted
    if storage_ok:
        persisted = bytes(u.mem_read(u.reg_read(UC_ARM_REG_R0), 916))
    ret(storage_ok)

def set_clock():
    raw = struct.pack('<II', u.reg_read(UC_ARM_REG_R0), u.reg_read(UC_ARM_REG_R1))
    clock_calls.append(struct.unpack('<H6B', raw))
    ret(clock_ok)

replace('watch_storage_load', load)
replace('watch_storage_save', save)
replace('watch_platform_clock_set', set_clock)

def call(name, *args):
    sp = 0x20270000
    for i, value in enumerate(args[:4]):
        u.reg_write(regs[i], value & 0xffffffff)
    for i, value in enumerate(args[4:]):
        u.mem_write(sp + i * 4, struct.pack('<I', value & 0xffffffff))
    u.reg_write(UC_ARM_REG_SP, sp)
    u.reg_write(UC_ARM_REG_LR, 0x0fff0001)
    u.emu_start(symbols[name] | 1, 0x0fff0000, count=10000000)
    if u.reg_read(UC_ARM_REG_PC) != 0x0fff0000:
        raise RuntimeError('Instruction limit: ' + name)
    return u.reg_read(UC_ARM_REG_R0)

def string(value):
    address = 0x210f0000
    u.mem_write(address, value.encode('ascii') + b'\0')
    return address

def cstring(address):
    return bytes(u.mem_read(address, 256)).split(b'\0')[0].decode()

def uint16(offset):
    return struct.unpack('<H', u.mem_read(symbols['watch_data'] + offset, 2))[0]

def check(condition, label):
    checks.append((bool(condition), label))
    print(('PASS: ' if condition else 'FAIL: ') + label, flush=True)

def time_call(name, year, month, day, hour=10, minute=0, second=0):
    # AAPCS passes this 8-byte value in R0/R1.
    args = struct.unpack('<II', struct.pack('<H6B', year, month, day, hour, minute, second, 0))
    return call(name, *args)

for folder, forbidden in [('models', ['lv_', 'HAL_', 'watch_storage_', 'viewmodels/', 'views/']),
                           ('viewmodels', ['lv_', 'HAL_', 'views/', 'platform/']),
                           ('services', ['lv_', 'HAL_', 'viewmodels/', 'views/']),
                           ('platform', ['watch_app_vm_', 'watch_faces_vm_'])]:
    bad = []
    for path in (ROOT / 'App' / folder).glob('*.[ch]'):
        source = path.read_text(encoding='utf-8')
        bad.extend((path.name, token) for token in forbidden if token in source)
    check(not bad, folder + ' preserves dependency boundaries: ' + str(bad))
view_sources = '\n'.join(p.read_text(encoding='utf-8') for p in (ROOT/'App'/'views').glob('*.[ch]'))
check('watch_data.' not in view_sources and 'watch_model_save(' not in view_sources,
      'Views do not mutate persistent state or call the repository')

call('watch_app_vm_init', 0)  # No display, widgets, LVGL initialization or UI observer.
check(call('watch_settings_vm_twelve') == 0 and uint16(908) == 2000, 'Headless startup supplies defaults')
check(not call('watch_water_vm_add', 1) and uint16(800) == 0, 'Unknown RTC blocks water commands')
time_call('watch_app_vm_time_changed', 2026, 10, 5)
check(call('watch_water_vm_add', 1) and uint16(800) == 200, 'Headless water command adds one glass')
call('watch_water_vm_add', -10)
check(uint16(800) == 0, 'Water cannot fall below zero')
call('watch_water_vm_add', 200)
check(uint16(800) == 20000, 'Water upper limit remains 20000 ml')
call('watch_water_vm_goal', -10000)
check(uint16(908) == 200, 'Water goal has a 200 ml lower limit')
call('watch_water_vm_goal', 10000)
check(uint16(908) == 6000, 'Water goal has a 6000 ml upper limit')
for expected in [30, 60, 120, 0]:
    call('watch_water_vm_reminder')
    check(uint16(910) == expected, 'Reminder cycles to %d minutes' % expected)

call('watch_calculator_vm_key', string('C'))
for key in ['2', '+', '3', '*', '4', '=']:
    call('watch_calculator_vm_key', string(key))
state = call('watch_calculator_vm_state')
check(cstring(state + 48) == '14', 'Headless calculator preserves operator precedence')
for key in ['+', '1', '=']:
    call('watch_calculator_vm_key', string(key))
check(cstring(state + 48) == '15', 'Calculator continues with the previous result')
for key in ['C', '1', '/', '0', '=']:
    call('watch_calculator_vm_key', string(key))
check(cstring(state + 48) == 'Gecersiz islem', 'Division by zero yields displayable error state')
call('watch_calculator_vm_key', string('8'))
check(cstring(state + 48) == '8', 'Calculator recovers from an invalid expression')

call('watch_notes_vm_open', 0)
call('watch_notes_vm_change', string('Draft only'))
check(cstring(symbols['watch_data'] + 28) == '', 'Editing a note leaves the model unchanged')
call('watch_notes_vm_open', 0)
check(cstring(call('watch_notes_vm_state') + 10) == '', 'Reopening discards an uncommitted draft')
call('watch_notes_vm_change', string('Saved note'))
check(call('watch_notes_vm_save') and cstring(symbols['watch_data'] + 28) == 'Saved note', 'Save commits a note through the repository')
check(not call('watch_notes_vm_delete') and cstring(symbols['watch_data'] + 28) == 'Saved note', 'First delete command asks for confirmation')
call('watch_notes_vm_change', string('Changed note'))
check(not call('watch_notes_vm_delete'), 'Changing the draft clears delete confirmation')
check(call('watch_notes_vm_delete') and cstring(symbols['watch_data'] + 28) == '', 'Confirmed delete commits an empty note')

call('watch_calendar_vm_init', 2000, 1)
call('watch_calendar_vm_move', 0)
calendar = call('watch_calendar_vm_state')
check(struct.unpack('<HB', u.mem_read(calendar, 3)) == (2000, 1), 'Calendar cannot move before January 2000')
call('watch_calendar_vm_init', 2099, 12)
call('watch_calendar_vm_move', 1)
check(struct.unpack('<HB', u.mem_read(calendar, 3)) == (2099, 12), 'Calendar cannot move past December 2099')
call('watch_clock_vm_open')
call('watch_clock_vm_date', 2024, 2, 31)
editor = call('watch_clock_vm_state')
check(bytes(u.mem_read(editor + 3, 1)) == b'\x1d', 'Clock editor clamps leap-year February to 29 days')
call('watch_clock_vm_date', 2025, 2, 29)
check(bytes(u.mem_read(editor + 3, 1)) == b'\x1c', 'Clock editor clamps ordinary February to 28 days')
before_calls = len(clock_calls)
check(not call('watch_clock_set_datetime', 2025, 2, 29, 12, 0, 0) and len(clock_calls) == before_calls,
      'Invalid dates never reach the RTC port')
clock_ok = False
check(not time_call('watch_clock_vm_save', 2026, 10, 5, 12, 30, 59), 'Clock editor exposes hardware write failure')
check(not bytes(u.mem_read(call('watch_app_vm_state') + 8, 1))[0], 'RTC write failure marks time unknown')
clock_ok = True
check(time_call('watch_clock_vm_save', 2026, 10, 5, 12, 30, 59) and clock_calls[-1][5] == 0,
      'Clock save retries successfully and resets seconds to zero')
check(bytes(u.mem_read(call('watch_app_vm_state') + 8, 1)) == b'\x01', 'Successful clock service updates the app ViewModel')

call('watch_alarm_vm_open', 0)
call('watch_alarm_vm_save', 12, 31)
time_call('watch_app_vm_set_datetime', 2026, 10, 5, 12, 31)
check(call('watch_alarm_next') == 0, 'Alarm command triggers through headless time updates')
call('watch_notification_vm_action', 1)
check(call('watch_alarm_next') == 0xffffffff, 'Snooze clears the current alarm notification')
time_call('watch_app_vm_set_datetime', 2026, 10, 5, 12, 36)
check(call('watch_alarm_next') == 0, 'Snoozed alarm returns after five minutes')
call('watch_notification_vm_action', 0)
check(not call('watch_notification_vm_alarm_active'), 'Stop command clears alarm output state')

old = bytes(u.mem_read(symbols['watch_data'] + 914, 1))
call('watch_faces_vm_open'); call('watch_faces_vm_choose', 3)
storage_ok = False
check(not call('watch_faces_vm_apply') and bytes(u.mem_read(symbols['watch_data'] + 914, 1)) == old,
      'Failed face save rolls back the active model selection')
check(bytes(u.mem_read(call('watch_app_vm_state') + 9, 1)) == b'\x00', 'Storage failure is exposed in app state')
storage_ok = True
check(call('watch_faces_vm_apply') and bytes(u.mem_read(symbols['watch_data'] + 914, 1)) == b'\x03',
      'Retry commits the retained face draft')
call('watch_notes_vm_open', 1); call('watch_notes_vm_change', string('Restart proof')); call('watch_notes_vm_save')
call('watch_settings_vm_toggle_theme')
saved = persisted
call('watch_app_vm_init', 0)
check(bytes(u.mem_read(symbols['watch_data'], 916)) == saved, 'Reinitialization reloads the unchanged 916-byte persistent layout')

def puzzle():
    address=call('watch_puzzle_vm_state')
    return list(u.mem_read(address,16)), u.mem_read(address+16,1)[0], u.mem_read(address+17,1)[0], struct.unpack('<I',u.mem_read(address+20,4))[0]

def solvable(tiles):
    inversions=sum(a>b for i,a in enumerate(tiles) if a for b in tiles[i+1:] if b)
    return (inversions+4-tiles.index(0)//4)%2==1

before_persistent=bytes(u.mem_read(symbols['watch_data'],916))
boards=set(); valid=True
for seed in range(512):
    call('watch_puzzle_vm_new',seed)
    tiles,blank,won,moves=puzzle()
    valid &= sorted(tiles)==list(range(16)) and blank==tiles.index(0) and solvable(tiles) and not won and moves==0
    boards.add(bytes(tiles))
check(valid,'512 shuffled games each contain 0-15 once, are solvable, unsolved and start at zero moves')
check(len(boards)==512,'Different seeds produce 512 distinct games')
call('watch_puzzle_vm_new',0); old=puzzle()[0]
call('watch_puzzle_vm_new',0)
check(puzzle()[0]!=old,'Reopening with identical tick entropy still creates a different board')

address=call('watch_puzzle_vm_state')
def fixture(tiles,blank,moves=0):
    u.mem_write(address,bytes(tiles)+bytes([blank,0])+b'\0\0'+struct.pack('<I',moves))

fixture([1,2,3,0,5,6,7,4,9,10,11,8,13,14,15,12],3)
before=puzzle()
check(not call('watch_puzzle_vm_move',4) and puzzle()==before,'Row-end to next-row-start is not an adjacent move')
check(not call('watch_puzzle_vm_move',0) and not call('watch_puzzle_vm_move',3) and not call('watch_puzzle_vm_move',16) and puzzle()==before,
      'Non-adjacent, blank and out-of-range taps do not change state or move count')
check(call('watch_puzzle_vm_move',7) and puzzle()[1]==7 and puzzle()[3]==1,'Vertical neighbor slides exactly one cell and counts once')

fixture(list(range(1,15))+[0,15],14,7)
check(call('watch_puzzle_vm_move',15) and puzzle()==(list(range(1,16))+[0],15,1,8),'Final horizontal move sets the win state with the correct move count')
check(not call('watch_puzzle_vm_move',14) and puzzle()[3]==8,'Completed board stays solved until a new game')
call('watch_puzzle_vm_new',42)
check(not puzzle()[2] and puzzle()[3]==0,'Restart clears win state and moves')
valid=True
for step in range(1024):
    tiles,blank,won,moves=puzzle()
    if won: break
    neighbors=[i for i in range(16) if abs(i//4-blank//4)+abs(i%4-blank%4)==1]
    slot=neighbors[step%len(neighbors)]; value=tiles[slot]
    accepted=call('watch_puzzle_vm_move',slot)
    after,new_blank,new_won,new_moves=puzzle()
    valid &= accepted and after[blank]==value and after[slot]==0 and new_blank==slot and new_moves==moves+1 and solvable(after)
check(valid,'1024 legal moves preserve tile identity, solvability, blank position and counters')
check(bytes(u.mem_read(symbols['watch_data'],916))==before_persistent,'Playing and restarting never change persistent notes, alarms or settings')

report = {'passed': sum(v for v, _ in checks), 'failed': sum(not v for v, _ in checks), 'checks': checks,
          'elf_sha256': hashlib.sha256((ROOT/'Debug'/'Smartwatch.elf').read_bytes()).hexdigest(),
          'scope': 'Compiled ARM Models/ViewModels; LVGL never initialized; flash and RTC ports are fakes.'}
(ROOT/'output'/'verify-viewmodels.json').write_text(json.dumps(report, indent=2))
print(json.dumps(report, indent=2), flush=True)
sys.exit(1 if report['failed'] else 0)
