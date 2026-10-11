"""Exercise real ARM display scheduling and LSM6DSOX adapter with hardware I/O fakes."""
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
    types.setdefault(die.attributes['DW_AT_name'].value.decode(),die)
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
 gfx0=offset('GFXMMU_HandleTypeDef','Init.Buffers.Buf0Address');gfx1=offset('GFXMMU_HandleTypeDef','Init.Buffers.Buf1Address')
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
call('lv_init');call('lv_tick_set_cb',symbols['HAL_GetTick']);display=call('lv_display_create',480,480);timer=call('lv_display_get_refr_timer',display)
u.mem_write(symbols['hlcd_gfxmmu']+gfx0,struct.pack('<I',0x21000000));u.mem_write(symbols['hlcd_gfxmmu']+gfx1,struct.pack('<I',symbols['back_framebuffer']))
AREA=0x210f0000;PIXELS=symbols['draw_buffer']
u.mem_write(PIXELS,bytes(range(256))*4);u.mem_write(AREA,struct.pack('<iiii',96,104,111,119))
call('flush',display,AREA,PIXELS)
check(read32(symbols['pending_display'])==display and ready_count==0,'Last flush returns without waiting for vblank and retains buffer ownership')
call('watch_platform_display_poll');check(len(copies)==1 and ready_count==0,'Polling before reload neither copies nor releases a pending frame')
call('display_refresh',timer);check(render_count==0,'Refresh callback cannot render over a pending hidden buffer')
call('HAL_LTDC_ReloadEventCallback',symbols['hlcd_ltdc']);tick+=16;call('watch_platform_display_poll')
check(ready_count==1 and read32(symbols['pending_display'])==0,'Reload completion releases exactly one pending frame in the main loop')
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
# LSM6DSOX register adapter: no board sensor is assumed.
connected=False;reads=[];writes=[];raw=(0,-6557,15016);sensor_ready=True
replace('BSP_I2C2_Init',lambda:ret())
def sensor_read():
 addr=u.reg_read(UC_ARM_REG_R1);reg=u.reg_read(UC_ARM_REG_R2);sp=u.reg_read(UC_ARM_REG_SP);dest=read32(sp);length=read32(sp+4);timeout=read32(sp+8)
 reads.append((addr,reg,length,timeout));assert timeout<=3
 if not connected or addr!=0x6A*2:ret(1);return
 data={0x0F:b'\x6c',0x1E:bytes([int(sensor_ready)]),0x28:struct.pack('<hhh',*raw)}[reg]
 u.mem_write(dest,data[:length]);ret()
def sensor_write():
 addr=u.reg_read(UC_ARM_REG_R1);reg=u.reg_read(UC_ARM_REG_R2);sp=u.reg_read(UC_ARM_REG_SP);data=bytes(u.mem_read(read32(sp),read32(sp+4)))
 assert read32(sp+8)<=3;writes.append((addr,reg,data));ret(0 if connected else 1)
replace('HAL_I2C_Mem_Read',sensor_read);replace('HAL_I2C_Mem_Write',sensor_write)
check(call('watch_motion_start')==0 and len(reads)==2,'Absent sensor probes both legal addresses with bounded timeouts')
connected=True;check(call('watch_motion_start')==1 and writes[-2:]==[(0xD4,0x12,b'\x44'),(0xD4,0x10,b'\x40')],'Detected LSM6DSOX enables BDU, register increment and 104Hz +/-2g acceleration')
check(call('watch_motion_read',AREA)==1 and struct.unpack('<hhh',u.mem_read(AREA,6))==(0,-399,915),'Acceleration adapter reads all three axes and converts signed raw values to milli-g')
sensor_ready=False;check(call('watch_motion_read',AREA)==0,'Stale acceleration samples are not consumed')
call('watch_motion_stop');check(writes[-1]==(0xD4,0x10,b'\x00') and call('watch_motion_read',AREA)==0,'Leaving motion control powers down acceleration and stops reads')
report={'passed':sum(ok for ok,_ in checks),'failed':sum(not ok for ok,_ in checks),'checks':checks,'elf_sha256':hashlib.sha256((ROOT/'Debug/Smartwatch.elf').read_bytes()).hexdigest(),'scope':'Real ARM scheduling/dirty copies and LSM6DSOX registers; DMA/LTDC/I2C primitives simulated. Hardware timing, GFXMMU mapping and a physical sensor are not verified.'}
(ROOT/'output/verify-game-platform.json').write_text(json.dumps(report,indent=2)+'\n');raise SystemExit(bool(report['failed']))
