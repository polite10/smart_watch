"""Focused ARM checks for clock gestures, readable information and game updates.

Uses the existing UI harness once. Does not rerun the full regression suite.
Hardware and GPU configuration are deliberately kept unchanged.
"""
from pathlib import Path
import json, struct, hashlib

ROOT=Path(__file__).resolve().parents[1]
harness=ROOT/'output/verify-round-ui.py'
c={'__file__':str(harness),'__name__':'refinements_harness'}
exec(compile(harness.read_text(encoding='utf-8').split("snapshot('home')")[0],str(harness),'exec'),c)
call,u,symbols,pointer=(c[k] for k in ('call','u','symbols','pointer'))
checks=[]
def check(ok,label):
    checks.append((bool(ok),label));print(('PASS' if ok else 'FAIL')+': '+label,flush=True)
def signed(value):return struct.unpack('<i',struct.pack('<I',value))[0]
def write_float(addr,*values):u.mem_write(addr,struct.pack('<'+'f'*len(values),*values))
def snapshot(name):
    c['snapshot']('refinement-'+name,0)
def tick(ms):
    c['tick']+=ms;call('arcade_tick',0)

# Four independent contacts: both screens follow the finger's dominant axis.
for dx,dy in [(-70,0),(70,0),(0,-70),(0,70)]:
    c['load'](0);c['settle']();c['sample'](False)
    c['sample'](True,240,240);c['sample'](True,240+dx,240+dy,40)
    c['settle'](100)
    call('lv_obj_update_layout',c['screens'](0));call('lv_obj_update_layout',c['screens'](1))
    x=signed(call('lv_obj_get_x',c['screens'](0)))
    y=signed(call('lv_obj_get_y',c['screens'](0)))
    check(c['active']()==c['screens'](1) and (x*dx>0 if dx else y*dy>0),
          'Clock swipe follows (%d,%d) finger direction'%(dx,dy))
    c['settle']();c['sample'](False)

call('watch_faces_vm_open');call('watch_faces_vm_choose',2);call('watch_faces_vm_apply')
c['load'](0);call('watch_ui_set_datetime',2026,10,12,12,0,0);call('watch_faces_apply_saved')
root=pointer('face_roots',2)
order=[call('lv_obj_get_child',root,i) for i in range(call('lv_obj_get_child_count',root))]
check(all(order.index(pointer(label,2))>order.index(pointer('minute_hand')) for label in ('face_date','face_water','face_battery')),
      'Classic information labels are drawn above the hands')
snapshot('clock-noon')
call('watch_ui_set_datetime',2026,10,12,12,30,0);snapshot('clock-half-hour')

c['load'](1);call('watch_navigation_show',6);c['settle'](100)
translation=call('lv_obj_get_style_prop',pointer('calc_label'),0,108)
check(40<translation<64,'App content remains visibly displaced after 100 ms')
c['settle'](140)
check(0<call('lv_obj_get_style_prop',pointer('calc_label'),0,108)<40,
      'App opening retains a visible intermediate position halfway through')
c['settle'](220)
check(call('lv_obj_get_style_prop',pointer('calc_label'),0,108)==0,
      'App opening finishes cleanly after its 440 ms motion')

c['load'](1);call('puzzle_open');c['settle']()
puzzle=call('watch_puzzle_vm_state')
board=bytes(u.mem_read(puzzle,16));blank=board.index(0)
children=[call('lv_obj_get_child',c['screens'](14),i) for i in range(call('lv_obj_get_child_count',c['screens'](14)))]
check(len(children)==20,'Puzzle has its backdrop and fifteen stones without a new-game button')
slot=next(i for i in range(16) if abs(i//4-blank//4)+abs(i%4-blank%4)==1)
value=board[slot];call('lv_obj_send_event',pointer('puzzle_tiles',value),10,0);c['settle'](200)
check(u.mem_read(puzzle+blank,1)[0]==value and c['read32'](puzzle+20)==1,'Stone moves still follow the original puzzle rules')
snapshot('puzzle-stones')
c['tap'](240,428)
check(c['read32'](puzzle+20)==1,'Former new-game area does not reset the puzzle')
old=bytes(u.mem_read(puzzle,16));c['load'](1);call('puzzle_open');c['settle']()
check(bytes(u.mem_read(puzzle,16))!=old and c['read32'](puzzle+20)==0,'Leaving and reopening starts a new puzzle')

call('flappy_open');c['settle']()
state=call('watch_flappy_vm_state')
def phase():return u.mem_read(state+46,1)[0]
def score():return struct.unpack('<H',u.mem_read(state+44,2))[0]
call('lv_obj_update_layout',c['screens'](15))
cloud=call('lv_obj_get_x',pointer('clouds'));far=call('lv_obj_get_x',pointer('hills'));near=call('lv_obj_get_x',pointer('near_hills'))
for _ in range(10):tick(40)
call('lv_obj_update_layout',c['screens'](15))
deltas=[(signed(v)-signed(call('lv_obj_get_x',pointer(n))))%period for n,v,period in [('clouds',cloud,660),('hills',far,720),('near_hills',near,720)]]
check(phase()==0 and deltas[0]>=18 and len(set(deltas))==3,
      'Ready screen visibly scrolls clouds and two hill layers at distinct speeds')
call('watch_flappy_vm_flap');call('watch_flappy_vm_pause')
frozen=bytes(u.mem_read(state,68));step=c['read32'](symbols['scenery_steps']);tick(160)
check(bytes(u.mem_read(state,68))==frozen and c['read32'](symbols['scenery_steps'])==step,'Pausing preserves physics and parallax')

# Safe pipe fixture: the collectible's visible center is shared with the model.
call('watch_flappy_vm_new',23);call('watch_flappy_vm_flap')
write_float(state,235,-0.25);u.mem_write(state+8,struct.pack('<ffBBBB',62.2,235,1,1,0,0))
call('watch_flappy_vm_step')
check(phase()==1 and score()==1 and u.mem_read(state+18,1)[0]==1,
      'Touching a flower awards exactly one bonus point and keeps flying')
write_float(state+4,-0.25);call('watch_flappy_vm_step')
check(score()==1,'A collected flower cannot award again')
# Reset the scored bit and pass another ordinary obstacle independently.
u.mem_write(state+8,struct.pack('<ffBBBB',108,235,0,0,0,0));write_float(state+4,-0.25);call('watch_flappy_vm_step')
check(score()==2,'Passing the same pipe still awards the normal point')

call('watch_flappy_vm_new',31);call('watch_flappy_vm_flap')
# Place a bee in a free corridor, away from both neighboring pipe caps.
u.mem_write(state+8,struct.pack('<ffBBBB',62.2,215,1,0,0,1))
scratch=0x210f0000
u.mem_write(state+52,struct.pack('<I',1));call('watch_flappy_bee',state,0,scratch,scratch+4)
bx,by=struct.unpack('<ff',u.mem_read(scratch,8))
u.mem_write(state+52,struct.pack('<I',0));write_float(state,by,-0.25)
call('watch_flappy_vm_step')
check(156<bx<184 and phase()==3,'Contact with a bee ends the run')

# Examine only spawned obstacles: successive flowers must be 2--5 pipes apart.
call('watch_flappy_vm_new',47);call('watch_flappy_vm_flap')
flowers=[]
for i in range(3):
    if u.mem_read(state+8+12*i+9,1)[0]:flowers.append(i+1)
for _ in range(24):
    write_float(state,235,-0.25);write_float(state+8,23)
    call('watch_flappy_vm_step')
    if u.mem_read(state+17,1)[0]:flowers.append(c['read32'](state+56))
check(len(flowers)>=4 and all(2<=b-a<=5 for a,b in zip(flowers,flowers[1:])),
      'Collectible flowers appear every two to five newly spawned obstacles')

# A composed still shows the new bee and collectible together.
call('watch_flappy_vm_new',59);u.mem_write(state+52,struct.pack('<I',130))
u.mem_write(state+8,struct.pack('<ffBBBB',145,215,0,0,0,1))
u.mem_write(state+20,struct.pack('<ffBBBB',320,260,0,1,0,0));call('flappy_render')
snapshot('flappy-bee-flower')

# Real transport counters feed the replacement launcher badge and detail page.
c['load'](1)
for name,value in [('uart_completed',16),('uart_bytes',1792),('uart_message_bytes',112),('uart_errors',0),('led_on',1)]:
    u.mem_write(symbols[name],struct.pack('<I',value))
call('HAL_UART_TxCpltCallback',symbols['led_demo_uart'])
call('dma_refresh',0)
check(c['text']('dma_badge')=='DMA\n112 B' and '1904 bayt' in c['text']('dma_details'),
      'DMA UI reads real completed transfers, message length and byte counters')
call('lv_obj_send_event',pointer('menu_buttons',6),10,0);c['settle']()
check(c['active']()==c['screens'](19),'Former clock button opens DMA details')
snapshot('dma-details');c['swipe_back']()
check(c['active']()==c['screens'](1),'DMA details retain the existing swipe-back navigation')
call('dma_refresh',0);snapshot('dma-menu')

call('snake_open');c['settle']();snake=call('watch_snake_vm_state')
bugs=[tuple(u.mem_read(snake+664+2*i,2)) for i in range(u.mem_read(snake+670,1)[0])]
body=[tuple(u.mem_read(snake+2*i,2)) for i in range(4)];food=tuple(u.mem_read(snake+648,2))
check(len(bugs)==3 and len(set(bugs))==3 and all(b not in body and b!=food for b in bugs),
      'Snake insects spawn separately, clear of the starting body and food')
snapshot('snake-garden')
call('watch_snake_vm_start')
u.mem_write(snake+664,bytes([9,9]));call('watch_snake_vm_step')
check(u.mem_read(snake+657,1)[0]==3,'Snake contact with an insect ends the game')
call('watch_snake_vm_new',73);u.mem_write(snake+670,b'\0');call('watch_snake_vm_start')
u.mem_write(snake,bytes([17,9]));u.mem_write(snake+648,bytes([1,1]));call('watch_snake_vm_step')
check(tuple(u.mem_read(snake,2))==(0,9) and u.mem_read(snake+657,1)[0]==1,
      'Existing Snake edge wrapping still works')
report={'passed':sum(ok for ok,_ in checks),'failed':sum(not ok for ok,_ in checks),'checks':checks,
        'elf_sha256':hashlib.sha256((ROOT/'Debug/Smartwatch.elf').read_bytes()).hexdigest(),
        'firmware_sha256':hashlib.sha256((ROOT/'Firmware/Smartwatch.bin').read_bytes()).hexdigest(),
        'scope':'Focused real ARM model/input/UI checks; software render for previews, no exhaustive regression rerun.'}
(ROOT/'output/verify-refinements.json').write_text(json.dumps(report,indent=2)+'\n')
raise SystemExit(bool(report['failed']))
