"""Exercise real ARM display scheduling with hardware I/O fakes."""
import sys,struct,json,hashlib
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'tmp/simulator-libs'))
from unicorn import Uc,UC_ARCH_ARM,UC_MODE_THUMB,UC_MODE_MCLASS,UC_HOOK_CODE
from unicorn.arm_const import *
from elftools.elf.elffile import ELFFile
u=Uc(UC_ARCH_ARM,UC_MODE_THUMB|UC_MODE_MCLASS);u.ctl_set_cpu_model(UC_CPU_ARM_CORTEX_M33)
for addr,size in [(0x08000000,0x400000),(0x20000000,0x280000),(0x21000000,0x100000),(0x24000000,0x200000),(0x24400000,0x200000),(0x0fff0000,0x1000)]:u.mem_map(addr,size)
with (ROOT/'Debug/Smartwatch.elf').open('rb') as f:
 elf=ELFFile(f);symbols={s.name:s['st_value'] for s in elf.get_section_by_name('.symtab').iter_symbols() if s.name}
 for seg in elf.iter_segments():
  if seg['p_type']=='PT_LOAD':u.mem_write(seg['p_vaddr'],seg.data())
 # Read field offsets from this ELF's debug types, including short-enum ABI.
 dwarf=elf.get_dwarf_info();types={}
 for cu in dwarf.iter_CUs():
  for die in cu.iter_DIEs():
   if die.tag in ('DW_TAG_typedef','DW_TAG_structure_type') and 'DW_AT_name' in die.attributes:
    name=die.attributes['DW_AT_name'].value.decode()
    if name not in types or (die.tag=='DW_TAG_structure_type' and die.has_children):types[name]=die
 def resolve(die):
  while die.tag in ('DW_TAG_typedef','DW_TAG_const_type'):die=die.get_DIE_from_attribute('DW_AT_type')
  return die
 def offset(type_name,path):
  die=resolve(types[type_name]);total=0
  for part in path.split('.'):
   index=0
   if '[' in part:part,number=part.split('[');index=int(number[:-1])
   child=next(c for c in die.iter_children() if c.attributes.get('DW_AT_name') and c.attributes['DW_AT_name'].value.decode()==part)
   total+=child.attributes['DW_AT_data_member_location'].value;die=resolve(child.get_DIE_from_attribute('DW_AT_type'))
   if die.tag=='DW_TAG_array_type':
    die=resolve(die.get_DIE_from_attribute('DW_AT_type'));total+=index*die.attributes['DW_AT_byte_size'].value
  return total
 dma_input=offset('DMA2D_HandleTypeDef','LayerCfg[1].InputOffset');dma_output=offset('DMA2D_HandleTypeDef','Init.OutputOffset')
 dma_format=offset('DMA2D_HandleTypeDef','LayerCfg[1].InputColorMode')
 blend_fields={name:offset('lv_draw_sw_blend_image_dsc_t',name) for name in ['dest_buf','dest_w','dest_h','dest_stride','mask_buf','src_buf','src_stride','src_color_format','opa','blend_mode']}
 gfx0=offset('GFXMMU_HandleTypeDef','Init.Buffers.Buf0Address');gfx1=offset('GFXMMU_HandleTypeDef','Init.Buffers.Buf1Address')
 timer_last_run=offset('_lv_timer_t','last_run')
u.reg_write(UC_ARM_REG_C1_C0_2,0xf00000);u.reg_write(UC_ARM_REG_FPEXC,0x40000000)
regs=[UC_ARM_REG_R0,UC_ARM_REG_R1,UC_ARM_REG_R2,UC_ARM_REG_R3];tick=1000
checks=[]
def read32(addr):return struct.unpack('<I',u.mem_read(addr,4))[0]
def ret(v=0):u.reg_write(UC_ARM_REG_R0,v);u.reg_write(UC_ARM_REG_PC,u.reg_read(UC_ARM_REG_LR))
def replace(name,callback):
 a=symbols[name]&~1;u.hook_add(UC_HOOK_CODE,lambda uc,a,size,data:callback(),begin=a,end=a)
def call(name,*args):
 sp=0x20270000
 for reg,val in zip(regs,args):u.reg_write(reg,val)
 for i,val in enumerate(args[4:]):u.mem_write(sp+i*4,struct.pack('<I',val))
 u.reg_write(UC_ARM_REG_SP,sp);u.reg_write(UC_ARM_REG_LR,0x0fff0001);u.emu_start(symbols[name]|1,0x0fff0000,count=20000000)
 assert u.reg_read(UC_ARM_REG_PC)==0x0fff0000,name
 return u.reg_read(UC_ARM_REG_R0)
def check(ok,label):checks.append((bool(ok),label));print(('PASS: ' if ok else 'FAIL: ')+label,flush=True)
replace('HAL_GetTick',lambda:ret(tick))
for name in ['HAL_DMA2D_Init','HAL_DMA2D_ConfigLayer','HAL_DMA2D_PollForTransfer','HAL_LTDC_SetAddress_NoReload','HAL_LTDC_Reload']:replace(name,lambda:ret())
copies=[]
def dma_start():
 handle=u.reg_read(UC_ARM_REG_R0);source=u.reg_read(UC_ARM_REG_R1);dest=u.reg_read(UC_ARM_REG_R2);width=u.reg_read(UC_ARM_REG_R3)
 height=read32(u.reg_read(UC_ARM_REG_SP));inp=read32(handle+dma_input);out=read32(handle+dma_output)
 copies.append((source,dest,width,height,inp,out))
 if read32(handle+dma_format)==2:
  for y in range(height):
   pixels=struct.unpack('<'+'H'*width,u.mem_read(source+y*(width+inp)*2,width*2))
   row=bytearray()
   for p in pixels:
    r=(p>>11)&31;g=(p>>5)&63;b=p&31
    row.extend(((b<<3)|(b>>2),(g<<2)|(g>>4),(r<<3)|(r>>2),255))
   u.mem_write(dest+y*(width+out)*4,bytes(row))
 else:
  for y in range(height):u.mem_write(dest+y*(width+out)*4,bytes(u.mem_read(source+y*(width+inp)*4,width*4)))
 ret()
replace('HAL_DMA2D_Start',dma_start)
last=True;ready_count=0;render_count=0
replace('lv_display_flush_is_last',lambda:ret(int(last)))
def ready():
 global ready_count
 ready_count+=1;ret()
def render():
 global render_count
 render_count+=1;ret()
replace('lv_display_flush_ready',ready);replace('lv_display_refr_timer',render)
replace('lv_draw_nema_gfx_init',lambda:ret()) # GPU rendering is measured on the physical board.
call('lv_init');call('lv_tick_set_cb',symbols['HAL_GetTick']);display=call('lv_display_create',480,480);timer=call('lv_display_get_refr_timer',display)
u.mem_write(symbols['hlcd_gfxmmu']+gfx0,struct.pack('<I',0x21000000));u.mem_write(symbols['hlcd_gfxmmu']+gfx1,struct.pack('<I',symbols['back_framebuffer']))
AREA=0x210f0000;PIXELS=symbols['draw_buffer']
u.mem_write(PIXELS,bytes(range(256))*4);u.mem_write(AREA,struct.pack('<iiii',96,104,111,119))
call('flush',display,AREA,PIXELS)
check(read32(symbols['pending_display'])==display and ready_count==0,'Last flush returns without waiting for vblank and retains buffer ownership')
call('watch_platform_display_poll');check(len(copies)==1 and ready_count==0,'Polling before reload neither copies nor releases a pending frame')
call('display_refresh',timer);check(render_count==0,'Refresh callback cannot render over a pending hidden buffer')
call('HAL_LTDC_ReloadEventCallback',symbols['hlcd_ltdc']);tick+=16
# A callback gated just before presentation must not add a fresh refresh period.
u.mem_write(timer+timer_last_run,struct.pack('<I',tick))
call('watch_platform_display_poll')
check(ready_count==1 and read32(symbols['pending_display'])==0,'Reload completion releases exactly one pending frame in the main loop')
check(tick-read32(timer+timer_last_run)>16,'Queued refresh is immediately eligible after vblank even if its callback was recently gated')
check(read32(symbols['smartwatch_sync_bytes'])==1024,'A 16x16 update synchronizes 1024 bytes instead of the entire 737280-byte buffer')
check(copies[-1][2:]==(16,16,752,752),'Dirty synchronization uses correct input and output virtual strides')
check(bytes(u.mem_read(0x24000000+(104*768+96)*4,64))==bytes(range(64)),'The hidden buffer contains the presented pixels for the next partial frame')
call('display_refresh',timer);check(render_count==1,'Refresh resumes after buffer synchronization')
# Multiple partial flushes form one frame and preserve the two-buffer history.
last=False;u.mem_write(AREA,struct.pack('<iiii',20,20,29,29));call('flush',display,AREA,PIXELS)
last=True;u.mem_write(AREA,struct.pack('<iiii',25,20,34,29));call('flush',display,AREA,PIXELS)
call('HAL_LTDC_ReloadEventCallback',symbols['hlcd_ltdc']);tick+=16;call('watch_platform_display_poll')
check(read32(symbols['smartwatch_sync_bytes'])==600 and read32(symbols['smartwatch_frame_count'])==2,'Overlapping partial flushes synchronize their union once and increment one frame')
last=True;u.mem_write(AREA,struct.pack('<iiii',0,0,479,479));call('flush',display,AREA,0x21000000)
call('HAL_LTDC_ReloadEventCallback',symbols['hlcd_ltdc']);tick+=16;call('watch_platform_display_poll')
check(copies[-1][2:]==(512,360,0,0) and read32(symbols['smartwatch_sync_bytes'])==737280,'Full-screen changes use the smaller packed physical buffer copy')
# Execute the real blend adapter, with HAL's DMA operation faked at its boundary.
DSC=0x210e0000;SOURCE=0x21080000;DEST=0x21090000
u.mem_write(DSC,bytes(128));u.mem_write(SOURCE,struct.pack('<12H',0xf800,0x07e0,0x001f,0,0,0,0xffff,0,0x8410,0,0,0));u.mem_write(DEST,bytes([0xcc])*40)
for name,value in {'dest_buf':DEST,'dest_w':3,'dest_h':2,'dest_stride':20,'src_buf':SOURCE,'src_stride':12}.items():u.mem_write(DSC+blend_fields[name],struct.pack('<I',value))
u.mem_write(DSC+blend_fields['src_color_format'],b'\x12');u.mem_write(DSC+blend_fields['opa'],b'\xff')
check(call('watch_display_blend_rgb565',DSC)==1 and read32(symbols['smartwatch_rgb565_dma_transfers'])==1,'Opaque RGB565 rendering uses the DMA2D color converter and completes synchronously')
check(copies[-1][2:]==(3,2,3,2),'Color conversion respects padded source and destination row strides')
check(bytes(u.mem_read(DEST,12))==bytes((0,0,255,255,0,255,0,255,255,0,0,255)) and bytes(u.mem_read(DEST+20,8))==bytes((255,255,255,255,0,0,0,255)),'RGB565 converts red, green, blue, white and black to opaque ARGB8888')
check(bytes(u.mem_read(DEST+12,8))==bytes([0xcc])*8 and bytes(u.mem_read(DEST+32,8))==bytes([0xcc])*8,'Color conversion preserves padding outside the clipped draw rectangle')
before=len(copies)
u.mem_write(DSC+blend_fields['opa'],b'\x50');rejected=call('watch_display_blend_rgb565',DSC)==0
u.mem_write(DSC+blend_fields['opa'],b'\xff');u.mem_write(DSC+blend_fields['mask_buf'],struct.pack('<I',SOURCE));rejected &= call('watch_display_blend_rgb565',DSC)==0
u.mem_write(DSC+blend_fields['mask_buf'],bytes(4));u.mem_write(DSC+blend_fields['src_stride'],struct.pack('<I',5));rejected &= call('watch_display_blend_rgb565',DSC)==0
check(rejected and len(copies)==before,'Opacity, masks and unsupported strides return to software without starting DMA')
report={'passed':sum(ok for ok,_ in checks),'failed':sum(not ok for ok,_ in checks),'checks':checks,'elf_sha256':hashlib.sha256((ROOT/'Debug/Smartwatch.elf').read_bytes()).hexdigest(),'scope':'Real ARM scheduling/dirty copies; DMA/LTDC primitives simulated. Hardware timing and GFXMMU mapping are not verified.'}
(ROOT/'output/verify-game-platform.json').write_text(json.dumps(report,indent=2)+'\n');raise SystemExit(bool(report['failed']))
