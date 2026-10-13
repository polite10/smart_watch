"""Check lighter wheel input and shared scenery/folder cache on the built ARM ELF.

Uses the UI harness without running the other suites or rewriting their images.
GPU timing and actual panel pixels are verified separately on the attached board.
"""
from pathlib import Path
import hashlib
import json
import struct

ROOT = Path(__file__).resolve().parents[1]
harness = ROOT / 'output/verify-round-ui.py'
c = {'__file__': str(harness), '__name__': 'motion_harness'}
exec(compile(harness.read_text(encoding='utf-8').split("snapshot('home')")[0], str(harness), 'exec'), c)
call, u, symbols = (c[k] for k in ('call', 'u', 'symbols'))
checks = []

def check(condition, label):
    checks.append((bool(condition), label))
    print(('PASS' if condition else 'FAIL') + ': ' + label, flush=True)

def angle():
    return struct.unpack('<i', u.mem_read(symbols['ring_angle'], 4))[0]

call('watch_navigation_show', 1)
call('watch_menu_reset')
call('ring_layout', 0, 0)
check(call('watch_menu_ring_begin', 418, 240, 1000), 'Outer ring accepts a contact')
check(not call('watch_menu_ring_drag', 418, 246, 1020), 'Tiny 1.9 degree jitter remains a tap')
check(call('watch_menu_ring_drag', 418, 250, 1040), 'A light 3.2 degree gesture starts rotation')
check(34 <= angle() <= 36, 'Drag applies the modest ten percent gain')
call('watch_menu_ring_end', 1050)
c['settle'](300)
check(angle() % 400 == 0, 'Slow release settles in one of the original slots')

call('ring_layout', 0, 0)
call('watch_menu_ring_begin', 240, 62, 2000)
call('watch_menu_ring_drag', 366, 114, 2020)
released = angle()
call('watch_menu_ring_end', 2030)
c['settle'](620)
late = angle()
check(late != released and call('lv_anim_get', symbols['menu_buttons'], symbols['ring_layout']) != 0,
      'A fast flick keeps gliding beyond the previous 620 ms duration')
c['settle'](120)
check(angle() % 400 == 0 and call('lv_anim_get', symbols['menu_buttons'], symbols['ring_layout']) == 0,
      'Extended glide stops cleanly by 740 ms without looping')

call('watch_menu_reset')
call('folder_cached_frame', 0)
cache = symbols['folder_pixels']
closed = bytes(u.mem_read(cache, c['sizes']['folder_pixels']))
call('folder_cached_frame', 128)
middle = bytes(u.mem_read(cache, len(closed)))
check(closed != middle, 'Folder has distinct closed and intermediate alpha frames')
for cycle in range(4):
    call('folder_cached_frame', 0)
    call('flappy_open')
    check(c['read32'](symbols['folder_frame']) == 0xffffffff,
          'Opening Flappy invalidates the folder cache tag, cycle %d' % cycle)
    scenery = bytes(u.mem_read(cache, 213060))
    check(scenery != closed[:len(scenery)],
          'Flappy decals replace the shared pixels, cycle %d' % cycle)
    call('watch_navigation_show', 1)
    call('watch_menu_reset')
    call('folder_cached_frame', 0)
    check(bytes(u.mem_read(cache, len(closed))) == closed,
          'Returning from Flappy restores the exact closed folder pixels, cycle %d' % cycle)
    call('folder_cached_frame', 128)
    check(bytes(u.mem_read(cache, len(closed))) == middle,
          'Reopening restores the exact transparent intermediate frame, cycle %d' % cycle)

report = {
    'passed': sum(ok for ok, _ in checks), 'failed': sum(not ok for ok, _ in checks),
    'checks': checks,
    'elf_sha256': hashlib.sha256((ROOT/'Debug/Smartwatch.elf').read_bytes()).hexdigest(),
    'scope': 'Compiled ARM input and LZ4 decoding; panel timing verified on hardware.'
}
(ROOT/'output/verify-motion.json').write_text(json.dumps(report, indent=2)+'\n')
raise SystemExit(bool(report['failed']))
