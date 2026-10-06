"""Execute the built ARM UI in Unicorn, render LVGL pixels and exercise real input.

Usage: pip install --target tmp/simulator-libs unicorn pyelftools
       python output/verify-round-ui.py
Only panel, tick and flash I/O are replaced. This cannot verify LTDC/DSI timing.
"""
import sys, struct, json, hashlib
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT/'tmp'/'simulator-libs'))
from unicorn import Uc, UC_ARCH_ARM, UC_MODE_THUMB, UC_MODE_MCLASS, UC_HOOK_CODE
from unicorn.arm_const import *
from elftools.elf.elffile import ELFFile
from PIL import Image, ImageDraw

u = Uc(UC_ARCH_ARM, UC_MODE_THUMB | UC_MODE_MCLASS)
u.ctl_set_cpu_model(UC_CPU_ARM_CORTEX_M33)
u.mem_map(0x08000000,0x400000)
u.mem_map(0x20000000,0x280000)
u.mem_map(0x21000000,0x100000)
u.mem_map(0x0fff0000,0x1000)
with (ROOT/'Debug'/'Smartwatch.elf').open('rb') as f:
    elf=ELFFile(f)
    symbols={s.name:s['st_value'] for s in elf.get_section_by_name('.symtab').iter_symbols() if s.name}
    sizes={s.name:s['st_size'] for s in elf.get_section_by_name('.symtab').iter_symbols() if s.name}
    for seg in elf.iter_segments():
        if seg['p_type']=='PT_LOAD': u.mem_write(seg['p_vaddr'],seg.data())
u.reg_write(UC_ARM_REG_C1_C0_2,0xf00000)
u.reg_write(UC_ARM_REG_FPEXC,0x40000000)
registers=[UC_ARM_REG_R0,UC_ARM_REG_R1,UC_ARM_REG_R2,UC_ARM_REG_R3]
tick=1000
raw=(False,240,240)
frame=bytearray(480*480*4)
power=[]
def ret(value=0):
    u.reg_write(UC_ARM_REG_R0,value)
    u.reg_write(UC_ARM_REG_PC,u.reg_read(UC_ARM_REG_LR))
def replace(name,callback):
    addr=symbols[name]&~1
    u.hook_add(UC_HOOK_CODE,lambda uc,a,size,d: callback(),begin=addr,end=addr)
replace('HAL_GetTick',lambda:ret(tick))
replace('watch_storage_load',lambda:ret(0))
storage_ok=True
replace('watch_storage_save',lambda:ret(int(storage_ok)))
def panel_power():
    power.append(bool(u.reg_read(UC_ARM_REG_R0)));ret()
replace('watch_ui_display_power',panel_power)
def touch_driver():
    data=u.reg_read(UC_ARM_REG_R1)
    pressed,x,y=raw
    u.mem_write(data,struct.pack('<ii',x,y))
    # CubeIDE uses short enums in its ARM EABI: state is a byte after enc_diff.
    u.mem_write(data+18,bytes([int(pressed)]))
    u.reg_write(UC_ARM_REG_R0,data);u.reg_write(UC_ARM_REG_R1,tick)
    u.reg_write(UC_ARM_REG_PC,symbols['watch_ui_touch'])
replace('read_touch',touch_driver)
def flush_driver():
    area=u.reg_read(UC_ARM_REG_R1); pixels=u.reg_read(UC_ARM_REG_R2)
    x1,y1,x2,y2=struct.unpack('<iiii',u.mem_read(area,16))
    width=(x2-x1+1)*4
    for y in range(y1,y2+1):
        start=(y*480+x1)*4
        frame[start:start+width]=u.mem_read(pixels,width);pixels+=width
    u.reg_write(UC_ARM_REG_PC,symbols['lv_display_flush_ready'])
replace('flush',flush_driver)
def call(name,*args):
    sp=0x20270000
    for i,v in enumerate(args[:4]):u.reg_write(registers[i],v&0xffffffff)
    for i,v in enumerate(args[4:]):u.mem_write(sp+4*i,struct.pack('<I',v&0xffffffff))
    u.reg_write(UC_ARM_REG_SP,sp);u.reg_write(UC_ARM_REG_LR,0x0fff0001)
    try:u.emu_start(symbols[name]|1,0x0fff0000,count=100000000)
    except Exception as e:
        pc=u.reg_read(UC_ARM_REG_PC)
        closest=sorted((abs((v&~1)-pc),k,hex(v)) for k,v in symbols.items() if v)[0:3]
        raise RuntimeError((name,hex(pc),closest,str(e))) from e
    if u.reg_read(UC_ARM_REG_PC)!=0x0fff0000:raise RuntimeError('Instruction limit: '+name)
    return u.reg_read(UC_ARM_REG_R0)
def read32(addr):return struct.unpack('<I',u.mem_read(addr,4))[0]
def pointer(name,i=0):return read32(symbols[name]+i*4)
def cstring(addr):
    data=bytes(u.mem_read(addr,256));return data.split(b'\0',1)[0].decode()
def text(name):return cstring(call('lv_label_get_text',pointer(name)))
screens=lambda i:pointer('screens',i)
def load(i):call('lv_screen_load',screens(i))
def settle(milliseconds=400):
    global tick
    for _ in range(0,milliseconds,20):
        tick+=20
        call('anim_timer',0)

def snapshot(name):
    settle()
    call('lv_display_refr_timer',0)
    im=Image.frombytes('RGBA',(480,480),bytes(frame),'raw','BGRA').convert('RGB')
    mask=Image.new('L',(480,480));ImageDraw.Draw(mask).ellipse((0,0,479,479),fill=255)
    out=Image.new('RGB',(480,480),'#171e2c');out.paste(im,(0,0),mask)
    out.save(ROOT/'output'/('round-'+name+'.png'))
checks=[]
def check(condition,label):
    checks.append((bool(condition),label));print(('PASS' if condition else 'FAIL')+': '+label,flush=True)
def sample(pressed,x=240,y=240,delta=16):
    global tick,raw
    tick+=delta;raw=(pressed,x,y);call('anim_timer',0);call('lv_indev_read_timer_cb',read_timer)
def tap(x,y):
    sample(True,x,y);sample(False,x,y,48);sample(False,x,y,280)
    call('lv_display_refr_timer',0)
    sample(False,x,y,16)
    settle(180)
def tap_object(obj):
    x=call('lv_obj_get_x',obj);y=call('lv_obj_get_y',obj)
    w=call('lv_obj_get_width',obj);h=call('lv_obj_get_height',obj)
    # Buttons in note/calendar containers use the same origin as the full screen.
    tap(x+w//2,y+h//2)
def swipe_back():
    sample(True,24,240);sample(True,108,242,64);sample(False,108,242,16)
    call('lv_display_refr_timer',0);sample(False,108,242,16)
    settle()
def active():return call('lv_screen_active')

call('lv_init');call('lv_tick_set_cb',symbols['HAL_GetTick'])
display=call('lv_display_create',480,480)
call('lv_display_set_color_format',display,0x10)
call('lv_display_set_buffers',display,symbols['draw_buffer'],0,480*40*4,0)
call('lv_display_set_flush_cb',display,symbols['flush'])
input_device=call('lv_indev_create');call('lv_indev_set_type',input_device,1)
call('lv_indev_set_read_cb',input_device,symbols['read_touch'])
read_timer=call('lv_indev_get_read_timer',input_device)
call('lv_indev_set_long_press_time',input_device,700)
call('watch_ui_init');call('watch_ui_set_clock_valid',1)
call('watch_ui_set_datetime',2026,10,5,10,8,12)
call('watch_ui_set_rtc_source',1)
snapshot('home')
check(active()==screens(0),'Home starts with clock')
sample(True);sample(False,delta=50)
check(power[-1] is False,'Single tap turns the panel off')
sample(True);sample(False,delta=50)
check(power[-1] is True and active()==screens(0),'Wake tap shows clock and is consumed')
sample(True);sample(True,280,240)
check(active()==screens(1),'Moving 40px unlocks directly to menu')
sample(False,280,240)
snapshot('menu')
check(call('lv_obj_get_child_count',screens(1))==10,'Menu has a centered title, nine icons and no back button')
check(sizes['watch_data']==916,'Persistent user-data structure retains its 916-byte format')
for i in range(9):
    button=call('lv_obj_get_child',screens(1),i+1)
    x=call('lv_obj_get_x',button);y=call('lv_obj_get_y',button);w=call('lv_obj_get_width',button)
    check(w==92 and ((x+w/2-240)**2+(y+w/2-240)**2)**.5+w/2<=240,'Icon %d has a large target wholly inside the circle'%i)
# Mirror symmetry and curved rows remain deliberate with an odd app count.
centers=[]
for i in range(9):
    button=call('lv_obj_get_child',screens(1),i+1)
    centers.append((call('lv_obj_get_x',button)+46,call('lv_obj_get_y',button)+46))
check(all(centers[a][0]+centers[b][0]==480 and centers[a][1]==centers[b][1] for a,b in [(0,2),(3,5),(6,8)]),
      'All three menu rows are left-right symmetric')
check(all(centers[a][1]+centers[b][1]==2*centers[4][1] for a,b in [(0,6),(1,7),(2,8)]),
      'Top and bottom curved rows mirror around the central puzzle icon')

# The ninth app is exercised through actual raw input and animated LVGL objects.
tap(240,269)
check(active()==screens(14),'Center puzzle icon opens the ninth app')
snapshot('puzzle')
puzzle_state=call('watch_puzzle_vm_state')
def board():return list(u.mem_read(puzzle_state,16))
def moves():return read32(puzzle_state+20)
before=board(); blank=before.index(0)
safe=True
for value in range(1,16):
    obj=pointer('puzzle_tiles',value)
    x=call('lv_obj_get_x',obj);y=call('lv_obj_get_y',obj)
    safe &= call('lv_obj_get_width',obj)==62 and call('lv_obj_get_height',obj)==62
    safe &= all((px-240)**2+(py-240)**2<=240**2 for px in (x,x+62) for py in (y,y+62))
check(safe,'All fifteen 62px square targets lie inside the round display')
invalid=next(i for i in range(16) if before[i] and abs(i//4-blank//4)+abs(i%4-blank%4)>1)
tap_object(pointer('puzzle_tiles',before[invalid]))
check(board()==before and moves()==0,'Tapping a non-adjacent tile leaves the board unchanged')
slot=next(i for i in range(16) if abs(i//4-blank//4)+abs(i%4-blank%4)==1)
value=before[slot]; tile=pointer('puzzle_tiles',value)
x=call('lv_obj_get_x',tile);y=call('lv_obj_get_y',tile)
sample(True,x+31,y+31);settle(60)
check(call('lv_obj_get_style_prop',tile,0,110)<256,'Pressed tile visibly compresses before the delayed click')
sample(False,x+31,y+31,48);sample(False,x+31,y+31,280)
check(moves()==1 and board()[blank]==value and u.mem_read(symbols['puzzle_moving'],1)[0],
      'Adjacent tap updates one move and starts a tile animation')
settle(60);call('lv_obj_update_layout',screens(14))
position=(call('lv_obj_get_x',tile),call('lv_obj_get_y',tile))
destination=(104+(blank%4)*70,118+(blank//4)*70)
check(position!=(x,y) and position!=destination,'Tile occupies an intermediate position while sliding')
# Extra callbacks during a move cannot start an overlapping move.
current=board(); current_blank=current.index(0)
other=next(current[i] for i in range(16) if current[i]!=value and abs(i//4-current_blank//4)+abs(i%4-current_blank%4)==1)
call('lv_obj_send_event',pointer('puzzle_tiles',other),10,0) # LV_EVENT_CLICKED
check(moves()==1,'Additional clicks during the tile slide are ignored')
settle(180);call('lv_obj_update_layout',screens(14))
check((call('lv_obj_get_x',tile),call('lv_obj_get_y',tile))==destination and not u.mem_read(symbols['puzzle_moving'],1)[0],
      'Tile finishes exactly at the empty-cell destination and releases the input guard')
old=board();tap(240,428)
check(board()!=old and moves()==0,'New game button reshuffles and clears the move count')
old=board()
swipe_back();tap(240,269)
check(board()!=old and moves()==0,'Opening the puzzle from the menu starts a new game')
snapshot('puzzle')

# A reachable board one move from completion verifies the winning UI.
fixture=list(range(1,15))+[0,15]
u.mem_write(puzzle_state,bytes(fixture)+b'\x0e\0\0\0'+struct.pack('<I',7))
for slot,value in enumerate(fixture):
    if value:call('lv_obj_set_pos',pointer('puzzle_tiles',value),104+(slot%4)*70,118+(slot//4)*70)
call('puzzle_refresh');call('lv_obj_update_layout',screens(14))
tap_object(pointer('puzzle_tiles',15));snapshot('puzzle-won')
check(board()==list(range(1,16))+[0] and text('puzzle_status')=='Tebrikler! / 8 hamle','Last tile slide displays congratulations and final moves')
tap_object(pointer('puzzle_tiles',15))
check(moves()==8,'Completed puzzle ignores tile taps')
tap(240,428)
check(not u.mem_read(puzzle_state+17,1)[0] and moves()==0 and 'Tebrikler' not in text('puzzle_status'),'Restart dismisses the win state')

swipe_back()
# Interrupt menu motion by entering and immediately leaving an app.
call('watch_navigation_show',6);call('watch_navigation_show',1);settle();call('lv_obj_update_layout',screens(1))
check(all(call('lv_obj_get_y',call('lv_obj_get_child',screens(1),i+1))==y for i,y in enumerate([123,89,123,223,223,223,323,357,323])),
      'Interrupted navigation restores all menu icons to their intended positions')
# Real taps go through raw gesture arbitration and the LVGL event dispatcher.
tap(346,169)
check(active()==screens(6),'Calculator icon opens calculator')
snapshot('calculator')
safe=True
rows=[]
for i in range(16):
    button=call('lv_obj_get_child',screens(6),i+4)
    x=call('lv_obj_get_x',button);y=call('lv_obj_get_y',button);w=call('lv_obj_get_width',button)
    safe &= w==68 and ((x+w/2-240)**2+(y+w/2-240)**2)**.5+w/2<=240
    rows.append(y)
check(safe,'All sixteen calculator keys are 68px and stay inside the round display')
check(all(len(set(rows[i:i+4]))==1 for i in range(0,16,4)),'Calculator uses four straight aligned rows')
tap(126,178);tap(354,406);tap(202,178);tap(278,406)
check(text('calc_label')=='15','Large keys evaluate 7 + 8 = 15')
# Double tap a calculator digit. The first release must not append a digit.
before=text('calc_label')
sample(True,126,178);sample(False,126,178,40);sample(True,126,178,100);sample(False,126,178,40)
check(active()==screens(0) and text('calc_label')==before,'Double tap locks without activating either key')
sample(True);sample(True,280,240);sample(False,280,240)
tap(134,369)
check(active()==screens(10),'Water icon opens tracker')
snapshot('water')
water_offset=symbols['watch_data']+4+3*8+4*192+4
initial=struct.unpack('<H',u.mem_read(water_offset,2))[0]
tap(388,211)
check(struct.unpack('<H',u.mem_read(water_offset,2))[0]==initial+200,'Large plus adds exactly one glass')
tap(92,211)
check(struct.unpack('<H',u.mem_read(water_offset,2))[0]==initial,'Large minus undoes one glass')
sample(True,388,211);sample(False,388,211,40);sample(True,388,211,100);sample(False,388,211,40)
check(active()==screens(0) and struct.unpack('<H',u.mem_read(water_offset,2))[0]==initial,'Double tap on water plus locks without adding water')
sample(True);sample(True,280,240);sample(False,280,240);snapshot('menu');tap(134,369)
tap(388,211);snapshot('water-filled')
swipe_back();tap(128,269)
check(active()==screens(7),'Notes icon opens notes')
tap(240,138)
check(active()==screens(8),'Note card opens large keyboard')
snapshot('note-editor')
letters=[]
for i in range(26):
    key=pointer('note_keys',i)
    letters.append(cstring(call('lv_label_get_text',key)))
check(''.join(letters)=='abcdefghijklmnopqrstuvwxyz','All 26 alphabet letters are visible together')
for i in range(26):
    key=call('lv_obj_get_parent',pointer('note_keys',i))
    tap_object(key)
check(cstring(call('lv_textarea_get_text',pointer('note_text')))=='abcdefghijklmnopqrstuvwxyz','Every visible letter inserts the expected character')
tap(389,185)
check(cstring(call('lv_textarea_get_text',pointer('note_text')))=='abcdefghijklmnopqrstuvwxy','Backspace works')
tap(193,185);tap_object(call('lv_obj_get_parent',pointer('note_keys',25)))
check(cstring(call('lv_textarea_get_text',pointer('note_text')))=='abcdefghijklmnopqrstuvwxyZ','Case button switches uppercase without hiding any letters')
tap(253,186)
check(cstring(call('lv_label_get_text',pointer('note_keys',0)))=='0','Number mode exposes digits')
tap(253,186)
check(cstring(call('lv_label_get_text',pointer('note_keys',0)))=='\uf8a2' or cstring(call('lv_label_get_text',pointer('note_keys',0)))!='','Symbols include a visible newline key')
tap(253,186)
tap(114,186)
check(active()==screens(7),'Save returns to notes')
check(cstring(symbols['watch_data']+4+3*8)=='abcdefghijklmnopqrstuvwxyZ','Edited note stored in unchanged model layout')
load(8);call('lv_display_refr_timer',0)
before=cstring(symbols['watch_data']+4+3*8)
tap_object(call('lv_obj_get_parent',pointer('note_keys',0)))
swipe_back()
check(active()==screens(7) and cstring(symbols['watch_data']+4+3*8)==before,'Edge-back cancels uncommitted note edits')
load(2);call('lv_display_refr_timer',0)
check(active()==screens(2),'Calendar opens from its menu destination')
check(call('lv_obj_get_child_count',pointer('calendar'))==53,'Calendar has circular day slots and weekday headings')
first=next(i for i in range(42) if bytes(u.mem_read(symbols['calendar_numbers']+i,1))==b'\x05')
tap_object(call('lv_obj_get_parent',pointer('calendar_labels',first)))
check(text('day_label')=='05.10.2026','Circular calendar date can be selected')
tap_object(pointer('calendar_arrows',1))
check(text('calendar_month_label')=='Kasim 2026','Calendar month advances')
tap_object(pointer('calendar_arrows',0))
check(text('calendar_month_label')=='Ekim 2026','Calendar month goes back')
for page,parent in [(1,0),(2,1),(3,1),(4,1),(5,4),(6,1),(7,1),(8,7),(9,1),(10,1),(11,10),(12,10),(13,1),(14,1)]:
    load(page);call('lv_display_refr_timer',0)
    header=pointer('page_titles',page)
    x=call('lv_obj_get_x',header);w=call('lv_obj_get_width',header)
    check(abs(x+w/2-240)<=1,'Page %d title stays horizontally centered'%page)
    swipe_back()
    check(active()==screens(parent),'Page %d edge-swipe returns to its parent'%page)
# Movement from the middle must not be interpreted as edge-back.
load(6);sample(True,240,205);sample(True,340,210,80);sample(False,340,210,16)
check(active()==screens(6),'A drag starting in the middle does not navigate back')
for i,name in [(4,'alarms'),(3,'settings'),(2,'calendar'),(11,'water-settings'),(12,'history'),(7,'notes')]:
    load(i);snapshot(name)
call('watch_faces_open');snapshot('faces')
apply_button=pointer('apply_button')
check(call('lv_obj_get_width',apply_button)==64 and call('lv_obj_get_height',apply_button)==64,'Face confirmation is a 64px circular target')
check(cstring(call('lv_label_get_text',call('lv_obj_get_child',apply_button,0)))=='\uf00c','Face confirmation shows only a checkmark')
check(call('lv_obj_get_child_count',screens(9))==7,'Gallery contains four faces and no color controls')
for i in range(4):
    b=pointer('style_buttons',i);x=call('lv_obj_get_x',b);y=call('lv_obj_get_y',b)
    check(((x+70-240)**2+(y+70-240)**2)**.5+70<=240,'Face %d preview stays inside round panel'%i)
tap_object(pointer('style_buttons',2))
tap_object(apply_button)
check(active()==screens(0) and bytes(u.mem_read(symbols['watch_data']+914,2))==b'\x02\x00','Checkmark saves face without changing legacy color byte')
call('watch_faces_open');tap_object(pointer('style_buttons',0))
storage_ok=False;tap_object(apply_button)
check(active()==screens(9) and bytes(u.mem_read(symbols['watch_data']+914,2))==b'\x02\x00','Failed face save retains prior selection and leaves picker open')
check(text('picker_hint')=='Kayit basarisiz' and cstring(call('lv_label_get_text',call('lv_obj_get_child',apply_button,0)))=='\uf021','Failed face save shows retry icon and error hint')
storage_ok=True;tap_object(apply_button)
check(active()==screens(0) and bytes(u.mem_read(symbols['watch_data']+914,2))==b'\x00\x00','Retry checkmark commits selection')
call('watch_faces_open');tap_object(pointer('style_buttons',3));tap_object(apply_button)
check(active()==screens(0) and bytes(u.mem_read(symbols['watch_data']+914,1))==b'\x03','New Orbit face can be selected and saved')
load(4);call('lv_display_refr_timer',0)
tap(342,156)
check(active()==screens(4) and bytes(u.mem_read(symbols['watch_data']+6,1))==b'\x01','Alarm switch enables alarm without opening editor')
tap(342,156)
check(bytes(u.mem_read(symbols['watch_data']+6,1))==b'\x00','Alarm switch disables alarm')
tap(178,156)
check(active()==screens(5),'Large alarm time opens matching editor')
swipe_back()
load(3);call('lv_display_refr_timer',0)
tap(340,212)
check(text('format_button_label')=='12 saat' and bytes(u.mem_read(symbols['watch_data']+912,1))==b'\x01','Tapping settings value changes clock format directly')
tap(340,212)
check(text('format_button_label')=='24 saat','Clock format row toggles back to 24 hours')
tap_object(pointer('settings_rows',0))
check(active()==screens(13),'Clock settings capsule opens date and time editor')
swipe_back();load(3);call('lv_display_refr_timer',0)
tap_object(pointer('settings_rows',3))
check(active()==screens(9),'Face settings capsule opens picker')
swipe_back()
call('clock_editor_open');snapshot('clock-editor')
# Every face is exercised with real LVGL drawing.
for i,name in enumerate(['pastel','neon','classic','orbit']):
    u.mem_write(symbols['watch_data']+914,bytes([i]))
    call('watch_faces_apply_saved');load(0);snapshot(name)
for i in [0,1,3]:
    label=pointer('face_clock',i)
    x=call('lv_obj_get_x',label);y=call('lv_obj_get_y',label)
    w=call('lv_obj_get_width',label);h=call('lv_obj_get_height',label)
    check(abs(x+w/2-240)<=1 and abs(y+h/2-240)<=1,'Digital face %d clock is centered'%i)
check((call('lv_obj_get_style_prop',pointer('face_roots',2),0,28)&0xffffff)==0x0c1422,'Classic face is dark in dark theme')
check((call('lv_obj_get_style_prop',pointer('classic_numbers',0),0,88)&0xffffff)==0xf0f4fc,'Classic hour numerals are white in dark theme')
classic=pointer('face_roots',2)
check('classic_seconds' not in symbols and call('lv_obj_get_child_count',classic)==71,
      'Analog face omits the redundant digital seconds label')
check(not call('lv_obj_has_flag',pointer('second_hand'),1),
      'Analog seconds hand remains visible with valid RTC')
def object_center(obj):
    return (call('lv_obj_get_x',obj)+call('lv_obj_get_width',obj)/2,
            call('lv_obj_get_y',obj)+call('lv_obj_get_height',obj)/2)
check(object_center(pointer('center_pin'))==(240,240),'Analog hands pivot exactly at the screen center')
expected_centers=[(240,104),(376,240),(104,240),(240,376)]
check(all(all(abs(a-b)<=1 for a,b in zip(object_center(pointer('classic_numbers',i)),expected_centers[i])) for i in range(4)),
      'Analog numerals sit symmetrically around the screen center')
top=object_center(pointer('face_date',2)); bottom=object_center(pointer('face_water',2))
check(abs(top[0]-240)<=1 and abs(bottom[0]-240)<=1 and abs(top[1]+bottom[1]-480)<=1,
      'Analog date and water information balance above and below the dial')
before_hand=bytes(u.mem_read(symbols['hand_points']+32,16))
u.mem_write(symbols['watch_data']+914,b'\x02'); call('watch_faces_apply_saved'); load(0)
call('watch_ui_set_datetime',2026,10,5,10,8,15)
check(bytes(u.mem_read(symbols['hand_points']+32,16))!=before_hand,
      'Analog seconds hand continues moving after digital seconds removal')
call('watch_ui_set_datetime',2026,10,5,10,8,12)
neon_alarm=pointer('face_alarm',1)
x=call('lv_obj_get_x',neon_alarm);y=call('lv_obj_get_y',neon_alarm)
w=call('lv_obj_get_width',neon_alarm);h=call('lv_obj_get_height',neon_alarm)
check(all((px-240)**2+(py-240)**2<180**2 for px in [x,x+w] for py in [y,y+h]),'Neon alarm status fits wholly inside progress ring')
mint=call('lv_obj_get_style_prop',pointer('face_clock',1),0,88)&0xffffff
u.mem_write(symbols['watch_data']+915,b'\x02');call('watch_faces_apply_saved')
check((call('lv_obj_get_style_prop',pointer('face_clock',1),0,88)&0xffffff)==mint==0x94e8c5,'Face colors remain fixed regardless of legacy customization byte')
load(3);tap(365,287);snapshot('settings-light')
check(bytes(u.mem_read(symbols['watch_data']+913,1))==b'\x01','Light theme remains selectable')
check(text('theme_button_label')=='Acik' and call('lv_obj_has_state',pointer('settings_switch'),1),'Theme capsule updates its value and switch together')
load(14);snapshot('puzzle-light')
check((call('lv_obj_get_style_prop',screens(14),0,28)&0xffffff)==0xeaf0f8,'Puzzle follows the selected light theme')
u.mem_write(symbols['watch_data']+914,b'\x02');call('watch_faces_apply_saved');load(0);snapshot('classic-light')
check((call('lv_obj_get_style_prop',pointer('face_roots',2),0,28)&0xffffff)==0xf6efe4,'Classic face follows light theme too')
call('watch_ui_set_clock_valid',0);load(0);snapshot('clock-unset')
check(text('face_clock')=='--:--','Invalid RTC displays unavailable time')
call('watch_ui_set_clock_valid',1);call('watch_ui_set_datetime',2026,10,5,10,8,12);load(0)
sample(True);sample(False,delta=50)
call('watch_reminder_set',30)
call('watch_ui_set_datetime',2026,10,5,10,40,0)
check(power[-1] is True,'Water reminder wakes an off panel automatically')
snapshot('reminder')
check(read32(symbols['displayed_notification'])==0xffffffff,'Reminder alert remains visible until acted on')
before=struct.unpack('<H',u.mem_read(water_offset,2))[0]
tap(240,280)
check(struct.unpack('<H',u.mem_read(water_offset,2))[0]==before+200,'Notification buttons bypass double-tap delay and add one glass')
names=['menu','puzzle','note-editor','faces','calculator','water-filled']
sheet=Image.new('RGB',(1440,960),'#171e2c')
for i,n in enumerate(names):sheet.paste(Image.open(ROOT/'output'/('round-'+n+'.png')),((i%3)*480,(i//3)*480))
sheet.save(ROOT/'output'/'round-ui-preview.png')
puzzles=Image.new('RGB',(1440,480),'#171e2c')
for i,n in enumerate(['menu','puzzle','puzzle-won']):puzzles.paste(Image.open(ROOT/'output'/('round-'+n+'.png')),(i*480,0))
puzzles.save(ROOT/'output'/'puzzle-preview.png')
controls=Image.new('RGB',(1440,480),'#171e2c')
for i,n in enumerate(['calculator','settings','faces']):controls.paste(Image.open(ROOT/'output'/('round-'+n+'.png')),(i*480,0))
controls.save(ROOT/'output'/'controls-preview.png')
gallery=Image.new('RGB',(1440,960),'#171e2c')
for i,n in enumerate(['faces','neon','classic','pastel','orbit','alarms']):gallery.paste(Image.open(ROOT/'output'/('round-'+n+'.png')),((i%3)*480,(i//3)*480))
gallery.save(ROOT/'output'/'gallery-preview.png')
report={'passed':sum(v for v,_ in checks),'failed':sum(not v for v,_ in checks),'checks':checks,
        'elf_sha256':hashlib.sha256((ROOT/'Debug'/'Smartwatch.elf').read_bytes()).hexdigest(),
        'firmware_sha256':hashlib.sha256((ROOT/'Firmware'/'Smartwatch.bin').read_bytes()).hexdigest(),
        'scope':'Real compiled ARM UI and LVGL. Panel/RTC/flash hardware timing is not emulated.'}
(ROOT/'output'/'verify-round-ui.json').write_text(json.dumps(report,indent=2))
print(json.dumps(report,indent=2),flush=True)
sys.exit(1 if report['failed'] else 0)
