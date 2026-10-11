"""Execute the real ARM flash journal; replace only flash programming/erase I/O."""
import sys,struct,json,hashlib,zlib
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tmp/simulator-libs'))
from unicorn import Uc,UC_ARCH_ARM,UC_MODE_THUMB,UC_MODE_MCLASS,UC_HOOK_CODE
from unicorn.arm_const import *
from elftools.elf.elffile import ELFFile
u=Uc(UC_ARCH_ARM,UC_MODE_THUMB|UC_MODE_MCLASS);u.ctl_set_cpu_model(UC_CPU_ARM_CORTEX_M33)
for address,size in [(0x08000000,0x400000),(0x20000000,0x280000),(0x21000000,0x100000),(0x40022000,0x1000),(0x0fff0000,0x1000)]:u.mem_map(address,size)
with (ROOT/'Debug/Smartwatch.elf').open('rb') as f:
 elf=ELFFile(f);symbols={s.name:s['st_value'] for s in elf.get_section_by_name('.symtab').iter_symbols() if s.name}
 for seg in elf.iter_segments():
  if seg['p_type']=='PT_LOAD':u.mem_write(seg['p_vaddr'],seg.data())
u.reg_write(UC_ARM_REG_C1_C0_2,0xf00000);u.reg_write(UC_ARM_REG_FPEXC,0x40000000)
regs=[UC_ARM_REG_R0,UC_ARM_REG_R1,UC_ARM_REG_R2,UC_ARM_REG_R3]
def ret(value=0):u.reg_write(UC_ARM_REG_R0,value);u.reg_write(UC_ARM_REG_PC,u.reg_read(UC_ARM_REG_LR))
def replace(name,callback):
 address=symbols[name]&~1;u.hook_add(UC_HOOK_CODE,lambda uc,a,size,data:callback(),begin=address,end=address)
program_calls=0;fail_after=None
def program():
 global program_calls
 program_calls+=1
 if fail_after is not None and program_calls>=fail_after:ret(1);return
 address=u.reg_read(UC_ARM_REG_R1);source=u.reg_read(UC_ARM_REG_R2)
 before=bytes(u.mem_read(address,16));data=bytes(u.mem_read(source,16))
 assert all((a&b)==b for a,b in zip(before,data)),'Attempted to overwrite programmed flash'
 u.mem_write(address,data);ret()
def erase():
 info=u.reg_read(UC_ARM_REG_R0);_,bank,page,count=struct.unpack('<IIII',u.mem_read(info,16));assert bank==2 and page in (254,255) and count==1
 u.mem_write(0x08200000+page*8192,b'\xff'*8192);ret()
for name in ['HAL_FLASH_Unlock','HAL_FLASH_Lock','HAL_ICACHE_Invalidate']:replace(name,lambda:ret())
replace('HAL_FLASH_Program',program);replace('HAL_FLASHEx_Erase',erase)
def call(name,*args):
 for reg,value in zip(regs,args):u.reg_write(reg,value)
 u.reg_write(UC_ARM_REG_SP,0x20270000);u.reg_write(UC_ARM_REG_LR,0x0fff0001)
 u.emu_start(symbols[name]|1,0x0fff0000,count=20000000)
 assert u.reg_read(UC_ARM_REG_PC)==0x0fff0000,name
 return u.reg_read(UC_ARM_REG_R0)
checks=[]
def check(ok,text):checks.append((bool(ok),text));print(('PASS: ' if ok else 'FAIL: ')+text,flush=True)
BASE=0x083fc000;DATA=0x21000000;RECORDS=DATA+2048;LOADED=DATA+4096
u.mem_write(BASE,b'\xff'*16384)
# Preserve arbitrary user bytes: legacy 916-byte payload and valid CRC.
legacy=bytearray((i*7+11)%256 for i in range(916));legacy[:4]=struct.pack('<I',1)
u.mem_write(BASE,struct.pack('<IIII',0x53574132,7,zlib.crc32(legacy),916)+bytes(legacy))
check(call('watch_storage_load',DATA)==1 and bytes(u.mem_read(DATA,916))==legacy,'Legacy version-1 journal loads all 916 user bytes unchanged')
call('watch_storage_records_load',RECORDS)
check(bytes(u.mem_read(RECORDS,12))==struct.pack('<IIHH',1,0,0,0),'Legacy record migrates to empty game records')
records=struct.pack('<IIHH',1,72,9,12);u.mem_write(RECORDS,records)
check(call('watch_storage_records_save',DATA,RECORDS)==1,'Extended settings and records commit atomically')
check(call('watch_storage_load',LOADED)==1 and bytes(u.mem_read(LOADED,916))==legacy,'Reload after migration preserves notes, alarms and all settings byte-for-byte')
call('watch_storage_records_load',RECORDS)
check(bytes(u.mem_read(RECORDS,12))==records,'Game records survive journal reinitialization')
count=program_calls;call('watch_storage_save',DATA)
check(program_calls==count,'Unchanged settings and records do not write flash')
# Generic settings saves must retain game records.
u.mem_write(DATA+904,struct.pack('<H',2400));changed=bytes(u.mem_read(DATA,916))
call('watch_storage_save',DATA);call('watch_storage_load',LOADED);call('watch_storage_records_load',RECORDS)
check(bytes(u.mem_read(LOADED,916))==changed and bytes(u.mem_read(RECORDS,12))==records,'Ordinary settings saves retain game records')
# Simulate a reset after a torn data write. The last committed header wins.
fail_after=program_calls+2;u.mem_write(RECORDS,struct.pack('<IIHH',1,60,15,20))
check(call('watch_storage_records_save',DATA,RECORDS)==0,'Interrupted flash programming reports failure')
call('watch_storage_load',LOADED);call('watch_storage_records_load',RECORDS)
check(bytes(u.mem_read(LOADED,916))==changed and bytes(u.mem_read(RECORDS,12))==records,'Torn write preserves the previous settings and records')
fail_after=None
for i in range(25):
 u.mem_write(RECORDS,struct.pack('<IIHH',1,60,15+i,20+i));assert call('watch_storage_records_save',DATA,RECORDS)==1
call('watch_storage_load',LOADED);call('watch_storage_records_load',RECORDS)
check(bytes(u.mem_read(LOADED,916))==changed and struct.unpack('<IIHH',u.mem_read(RECORDS,12))==(1,60,39,44),'Both journal pages rotate repeatedly without losing settings or records')
# Latest CRC failure must fall back to the previous committed snapshot.
latest=struct.unpack('<I',u.mem_read(symbols['latest_address'],4))[0];bad=bytearray(u.mem_read(latest+16,928));bad[-1]^=1;u.mem_write(latest+16,bytes(bad))
call('watch_storage_load',LOADED);call('watch_storage_records_load',RECORDS)
check(struct.unpack('<IIHH',u.mem_read(RECORDS,12))==(1,60,38,43),'Corrupt latest CRC falls back to the previous valid snapshot')
report={'passed':sum(ok for ok,_ in checks),'failed':sum(not ok for ok,_ in checks),'checks':checks,'elf_sha256':hashlib.sha256((ROOT/'Debug/Smartwatch.elf').read_bytes()).hexdigest(),'scope':'Real ARM storage journal with flash program/erase primitives simulated; physical flash timing and power loss are not verified.'}
(ROOT/'output/verify-game-storage.json').write_text(json.dumps(report,indent=2)+'\n')
raise SystemExit(bool(report['failed']))
