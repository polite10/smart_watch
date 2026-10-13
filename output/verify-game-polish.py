"""Focused ARM checks for staged bees, parallax and continuous Snake drawing."""
from pathlib import Path
import struct,json,hashlib
ROOT=Path(__file__).resolve().parents[1]
harness=ROOT/'output/verify-round-ui.py'
c={'__file__':str(harness),'__name__':'game_polish_harness'}
exec(compile(harness.read_text(encoding='utf-8').split("snapshot('home')")[0],str(harness),'exec'),c)
call,u,symbols,pointer=(c[n] for n in ('call','u','symbols','pointer'))
checks=[];densities={};scratch=0x210f0000
def check(ok,label):
    checks.append((bool(ok),label));print(('PASS: ' if ok else 'FAIL: ')+label,flush=True)
def floats(addr,*values):u.mem_write(addr,struct.pack('<'+'f'*len(values),*values))
def write32(addr,value):u.mem_write(addr,struct.pack('<I',value))
def snapshot(name):c['snapshot']('polish-'+name,0)
def tick(ms):c['tick']+=ms;call('arcade_tick',0)
state=call('watch_flappy_vm_state')
for score in (0,29,30,59,60,90):
    call('watch_flappy_new',state,73);call('watch_flappy_vm_flap')
    seen=[];separate=True;scheduled=[]
    for _ in range(84):
        u.mem_write(state+44,struct.pack('<H',score));floats(state,235,-.25);floats(state+8,23)
        call('watch_flappy_vm_step')
        flower,bee=u.mem_read(state+17,1)[0],u.mem_read(state+19,1)[0]
        separate &= not (flower and bee)
        if bee:
            seen.append(c['read32'](state+56));scheduled.append(c['read32'](state+64)-seen[-1])
    minimum,maximum=(6,8) if score<30 else (4,6) if score<60 else (3,4)
    densities[score]=len(seen)
    check(bool(seen) and all(minimum<=n<=maximum for n in scheduled) and separate,
          'Score %d schedules bees every %d--%d obstacles and keeps flower corridors free'%(score,minimum,maximum))
check(densities[0]<densities[30]<densities[60] and densities[60]==densities[90],
      'Actual bee density rises at 30 and 60, then stays capped')

call('watch_flappy_vm_new',91)
u.mem_write(state+8,struct.pack('<ffBBBB',200,235,1,0,0,1))
for score in (0,30,60):
    u.mem_write(state+44,struct.pack('<H',score));positions=[]
    for step in range(0,240,4):
        write32(state+52,step);assert call('watch_flappy_bee',state,0,scratch,scratch+4)
        positions.append(struct.unpack('<ff',u.mem_read(scratch,8)))
    check(max(x for x,y in positions)-min(x for x,y in positions)>10 and
          all(304<=x<=316 and abs(y-235)>=24 for x,y in positions),
          'Score %d bee patrol moves in both axes while preserving center escape lane'%score)

call('flappy_open');c['settle']();call('arcade_tick',0);call('lv_obj_update_layout',c['screens'](15))
initial=[call('lv_obj_get_x',pointer(n)) for n in ('clouds','hills','near_hills','ground_details')]
for _ in range(10):tick(40)
call('lv_obj_update_layout',c['screens'](15))
deltas=[(a-call('lv_obj_get_x',pointer(n)))%period for a,n,period in zip(initial,('clouds','hills','near_hills','ground_details'),(660,720,720,96))]
check(deltas==[32,14,38,54],'Cloud, mountain, forest and ground layers scroll at four distinct speeds')
snapshot('flappy-parallax')
call('watch_flappy_vm_flap');call('watch_flappy_vm_pause')
frozen=bytes(u.mem_read(state,68));steps=c['read32'](symbols['scenery_steps']);tick(160)
check(bytes(u.mem_read(state,68))==frozen and c['read32'](symbols['scenery_steps'])==steps,'Pause freezes the new bee schedule and parallax')
call('watch_flappy_vm_new',97);call('watch_flappy_vm_flap');u.mem_write(state+8,struct.pack('<ffBBBB',62.2,235,1,1,0,0));floats(state,235,-.25)
call('watch_flappy_vm_step')
check(struct.unpack('<H',u.mem_read(state+44,2))[0]==1 and u.mem_read(state+46,1)[0]==1,'Flowers retain their safe +1 bonus')
call('watch_flappy_vm_new',101);call('watch_flappy_vm_flap');u.mem_write(state+8,struct.pack('<ffBBBB',62.2,215,1,0,0,1))
write32(state+52,1);call('watch_flappy_bee',state,0,scratch,scratch+4);bx,by=struct.unpack('<ff',u.mem_read(scratch,8));write32(state+52,0);floats(state,by,-.25)
call('watch_flappy_vm_step');check(u.mem_read(state+46,1)[0]==3,'Moving bee contact still ends the run')

call('puzzle_open');c['settle']();puzzle_bg=bytes(u.mem_read(symbols['folder_pixels'],204800));snapshot('puzzle-workshop')
check(call('lv_obj_get_width',pointer('game_backgrounds'))==480,'Puzzle workshop covers the area around the board')
call('snake_open');c['settle']();snake_bg=bytes(u.mem_read(symbols['folder_pixels'],204800));snapshot('snake-garden')
check(snake_bg!=puzzle_bg and c['read32'](symbols['folder_frame'])==0xffffffff,'Snake garden replaces the shared backdrop and invalidates folder cache')
snake=call('watch_snake_vm_state');canvas=symbols['snake_pixels']
call('watch_snake_vm_start');tick(160);before=bytes(u.mem_read(canvas,288*288*2));tick(40)
check(bytes(u.mem_read(canvas,288*288*2))!=before and 0<c['read32'](symbols['smartwatch_snake_pixel_writes'])<10000,
      'Snake moves between grid ticks with bounded incremental drawing')
# Incremental erasure must match a fresh draw even at turns and all four seams.
clean=True
for direction in range(4):
    call('snake_open');call('watch_snake_vm_start');u.mem_write(snake+670,b'\0')
    cells=[(17-i,9) if direction==0 else (9,17-i) if direction==1 else (i,9) if direction==2 else (9,i) for i in range(4)]
    u.mem_write(snake,b''.join(bytes(p) for p in cells));u.mem_write(symbols['snake_from'],b''.join(bytes(p) for p in cells))
    u.mem_write(snake+648,bytes((1,1)));u.mem_write(snake+654,bytes((direction,direction,0)))
    u.mem_write(symbols['snake_from_direction'],bytes((direction,)));u.mem_write(symbols['snake_picture_valid'],b'\0');call('snake_render',256)
    call('watch_snake_vm_step');call('snake_render',128)
    incremental=bytes(u.mem_read(canvas,288*288*2));u.mem_write(symbols['snake_picture_valid'],b'\0');call('snake_render',128)
    clean &= incremental==bytes(u.mem_read(canvas,288*288*2))
check(clean,'Continuous Snake body wraps cleanly without leaving streaks at any seam')
call('snake_open');call('watch_snake_vm_start');u.mem_write(snake+670,b'\0')
u.mem_write(symbols['snake_picture_valid'],b'\0');call('snake_render',256)
call('watch_snake_vm_turn',0,-40);call('watch_snake_vm_step')
turn_clean=True
for fraction in (64,128,192,256):
    call('snake_render',fraction);incremental=bytes(u.mem_read(canvas,288*288*2))
    u.mem_write(symbols['snake_picture_valid'],b'\0');call('snake_render',fraction)
    turn_clean &= incremental==bytes(u.mem_read(canvas,288*288*2))
check(turn_clean,'Curved Snake turns restore the same pixels as a fresh drawing')
call('snake_open');call('watch_snake_vm_start');u.mem_write(snake+664,bytes((9,9)));call('watch_snake_vm_step')
check(u.mem_read(snake+657,1)[0]==3,'Detailed insects retain the original fatal collision rule')
call('puzzle_open');check(bytes(u.mem_read(symbols['folder_pixels'],204800))==puzzle_bg,'Reopening Puzzle restores its exact workshop backdrop')
call('flappy_open');check(c['active']()==c['screens'](15) and c['read32'](symbols['folder_frame'])==0xffffffff,'Returning to Flappy reloads its scenery without stale shared pixels')
# A small actual-LVGL preview makes all four parallax layers visible in motion.
frames=[];call('arcade_tick',0)
for _ in range(8):
    for _ in range(6):tick(40)
    call('lv_display_refr_timer',0)
    frames.append(c['Image'].frombytes('RGBA',(480,480),bytes(c['frame']),'raw','BGRA').convert('RGB'))
frames[0].save(ROOT/'output/flappy-polish-parallax.gif',save_all=True,append_images=frames[1:],duration=240,loop=0)
report={'passed':sum(ok for ok,_ in checks),'failed':sum(not ok for ok,_ in checks),'checks':checks,'bee_counts_over_84_spawns':densities,
        'elf_sha256':hashlib.sha256((ROOT/'Debug/Smartwatch.elf').read_bytes()).hexdigest(),
        'firmware_sha256':hashlib.sha256((ROOT/'Firmware/Smartwatch.bin').read_bytes()).hexdigest(),
        'scope':'Focused real ARM model and LVGL drawing; device timing is measured separately.'}
(ROOT/'output/verify-game-polish.json').write_text(json.dumps(report,indent=2)+'\n')
raise SystemExit(bool(report['failed']))
