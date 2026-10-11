"""Exercise Flappy spacing, collision boundaries and bird tilt in the built ARM UI.

Run after Build.ps1 with unicorn, pyelftools and Pillow installed.
Uses the existing UI harness without running its other tests or writing its reports.
"""
from pathlib import Path
import hashlib
import json
import struct

ROOT = Path(__file__).resolve().parents[1]
harness = ROOT / 'output/verify-round-ui.py'
context = {'__file__': str(harness), '__name__': 'flappy_harness'}
exec(compile(harness.read_text(encoding='utf-8').split("snapshot('home')")[0], str(harness), 'exec'), context)
call, pointer, u = (context[key] for key in ('call', 'pointer', 'u'))
checks = []

def check(condition, label):
    checks.append((bool(condition), label))
    print(('PASS' if condition else 'FAIL') + ': ' + label, flush=True)

call('flappy_open')
state = call('watch_flappy_vm_state')
bird = pointer('flappy_bird')
with (ROOT / 'Debug/Smartwatch.elf').open('rb') as source:
    dwarf = context['ELFFile'](source).get_dwarf_info()
    image_type = next(die for cu in dwarf.iter_CUs() for die in cu.iter_DIEs()
                      if die.tag == 'DW_TAG_structure_type'
                      and die.attributes.get('DW_AT_name')
                      and die.attributes['DW_AT_name'].value == b'_lv_image_t'
                      and die.has_children)
    rotation_offset = next(die.attributes['DW_AT_data_member_location'].value
                           for die in image_type.iter_children()
                           if die.attributes.get('DW_AT_name')
                           and die.attributes['DW_AT_name'].value == b'rotation')

def phase():
    return u.mem_read(state + 46, 1)[0]

def reset_running():
    call('watch_flappy_vm_new', 23)
    call('watch_flappy_vm_flap')

def angle():
    value = struct.unpack('<I', u.mem_read(bird + rotation_offset, 4))[0]
    return value - 3600 if value > 1800 else value

x = [struct.unpack('<f', u.mem_read(state + 8 + 12*i, 4))[0] for i in range(3)]
check(x == [350.0, 525.0, 700.0], 'Pipe pairs start 175px apart horizontally')
top, bottom = pointer('pipe_top'), pointer('pipe_bottom')
call('lv_obj_update_layout', context['screens'](15))
check(call('lv_obj_get_y', bottom) - (call('lv_obj_get_y', top) + call('lv_obj_get_height', top)) == 104,
      'Rendered vertical opening is 104px')
check(angle() == 0, 'Ready bird is level')
for offset, survives in [(-40, True), (40, True), (-41, False), (41, False)]:
    reset_running()
    u.mem_write(state, struct.pack('<ff', 235.0 + offset, -0.25))
    u.mem_write(state + 8, struct.pack('<ff', 170.0, 235.0))
    call('watch_flappy_vm_step')
    check((phase() == 1) == survives, 'Vertical collision boundary at %+dpx matches the opening' % offset)

reset_running()
u.mem_write(state + 8, struct.pack('<f', 23.0))
call('watch_flappy_vm_step')
x = [struct.unpack('<f', u.mem_read(state + 8 + 12*i, 4))[0] for i in range(3)]
check(175.0 <= x[0] - max(x[1:]) <= 177.3, 'Recycled pipe keeps the new horizontal spacing within one physics step')

reset_running()
call('flappy_render')
check(angle() == -240, 'Touch-down pitches the bird gently upward by 24 degrees')
poses = []
for velocity, expected in [(-4.4, -240), (0.0, 0), (5.0, 300)]:
    u.mem_write(state, struct.pack('<ff', 235.0, velocity))
    call('flappy_render')
    check(angle() == expected, 'Velocity %.1f sets the expected tilt' % velocity)
    call('lv_display_refr_timer', 0)
    image = context['Image'].frombytes('RGBA', (480, 480), bytes(context['frame']), 'raw', 'BGRA')
    poses.append(image.crop((118, 178, 222, 292)).convert('RGB'))
check(poses[0].tobytes() != poses[1].tobytes() and poses[1].tobytes() != poses[2].tobytes(),
      'LVGL renders distinct rising, level and falling sprite pixels')

reset_running()
call('flappy_render')
angles = [angle()]
for _ in range(19):
    call('watch_flappy_vm_step')
    call('flappy_render')
    angles.append(angle())
check(all(a <= b for a, b in zip(angles, angles[1:])) and angles[0] < 0 < angles[-1],
      'Bird tilts smoothly through the flight apex into descent')
check(max(b - a for a, b in zip(angles, angles[1:])) <= 16,
      'Normal flight changes tilt by at most 1.6 degrees per physics step')
call('watch_flappy_vm_pause')
paused = angle()
call('watch_flappy_vm_step')
call('flappy_render')
check(phase() == 2 and angle() == paused, 'Pause preserves the current flight pose')
call('flappy_open')
check(phase() == 0 and angle() == 0, 'Restart resets the pose to level')

sheet = context['Image'].new('RGB', (312, 140), '#171e2c')
draw = context['ImageDraw'].Draw(sheet)
for i, (pose, label) in enumerate(zip(poses, ['Rising -24 deg', 'Level 0 deg', 'Falling +30 deg'])):
    sheet.paste(pose, (i*104, 26))
    draw.text((i*104 + 4, 7), label, fill='#fff0c7')
sheet.save(ROOT / 'output/flappy-flight-preview.png')
report = {
    'passed': sum(ok for ok, _ in checks), 'failed': sum(not ok for ok, _ in checks), 'checks': checks,
    'spacing_px': 175, 'vertical_gap_px': 104, 'tilt_degrees': {'rise': -24, 'fall': 30},
    'elf_sha256': hashlib.sha256((ROOT / 'Debug/Smartwatch.elf').read_bytes()).hexdigest(),
    'firmware_sha256': hashlib.sha256((ROOT / 'Firmware/Smartwatch.bin').read_bytes()).hexdigest(),
    'scope': 'Real ARM model and LVGL renderer; panel hardware timing is measured separately.'
}
(ROOT / 'output/verify-flappy.json').write_text(json.dumps(report, indent=2) + '\n')
raise SystemExit(bool(report['failed']))
