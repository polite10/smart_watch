"""Build DeviceBoot/Duello against this checkout on macOS or Windows.

Invoked by each project's build.py. No board connection or flashing occurs.
"""
import argparse
import os
from pathlib import Path
import shutil
import subprocess
import sys


def toolchain(ide_root=None):
    executable='arm-none-eabi-gcc.exe' if os.name=='nt' else 'arm-none-eabi-gcc'
    if os.environ.get('ARM_GCC_BIN'):
        directory=Path(os.environ['ARM_GCC_BIN']).expanduser().resolve()
        if not (directory/executable).is_file():
            raise SystemExit('ARM_GCC_BIN does not contain '+executable)
        return directory
    roots=[Path(ide_root)] if ide_root else (
        sorted(Path('C:/ST').glob('STM32CubeIDE_*/STM32CubeIDE'),reverse=True) if os.name=='nt'
        else [Path('/Applications/STM32CubeIDE.app/Contents/Eclipse')])
    for root in roots:
        candidates=sorted((root/'plugins').glob('com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.*/tools/bin'),reverse=True)
        for directory in candidates:
            if (directory/executable).is_file():return directory
    compiler=shutil.which(executable)
    if compiler:return Path(compiler).resolve().parent
    raise SystemExit('ARM GCC not found; install CubeIDE or set ARM_GCC_BIN')


def main(project,target,sdk):
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--smartwatch-root',type=Path)
    parser.add_argument('--ide-root')
    parser.add_argument('--skip-sdk-build',action='store_true')
    args=parser.parse_args()
    project=Path(project).resolve();sdk=Path(sdk).resolve()
    bindir=toolchain(args.ide_root)
    suffix='.exe' if os.name=='nt' else ''
    def tool(name):return str(bindir/('arm-none-eabi-'+name+suffix))
    def run(command,**kwargs):subprocess.run(list(map(str,command)),check=True,cwd=project,**kwargs)
    common=['-mcpu=cortex-m33','-mthumb','-mfpu=fpv5-sp-d16','-mfloat-abi=hard',
            '-g3','-ffunction-sections','-fdata-sections','-Wall','-Wextra',
            '-DSTM32U5A9xx','-DUSE_HAL_DRIVER','--specs=nano.specs']
    includes=['Core/Inc','Drivers/CMSIS/Include','Drivers/CMSIS/Device/ST/STM32U5xx/Include',
              'Drivers/STM32U5xx_HAL_Driver/Inc']
    if target=='DeviceBoot':
        sources=[project/'src/boot.c',sdk.parent/'AppManager/src/am_manager.c',
                 sdk.parent/'AppManager/platform/am_nor_stm32.c',sdk.parent/'AppManager/platform/am_port.c',
                 sdk/'Core/Src/system_stm32u5xx.c',
                 sdk/'Example/Startup/startup_stm32u5a9njhxq.s',
                 sdk/'Example/User/syscalls.c',sdk/'Example/User/sysmem.c']
        for name in ('hal','hal_cortex','hal_gpio','hal_rcc','hal_rcc_ex','hal_pwr','hal_pwr_ex',
                     'hal_flash','hal_flash_ex','hal_uart','hal_uart_ex','hal_xspi'):
            sources.append(sdk/f'Drivers/STM32U5xx_HAL_Driver/Src/stm32u5xx_{name}.c')
        common+=['-Os','-flto','-fno-inline','-fno-ipa-cp','-fno-ipa-sra',
                 '-DDEVICE_BOOT_BUILD','-I'+str(project/'include')]
        linker=project/'DeviceBoot.ld';libraries=[]
    else:
        if not args.skip_sdk_build:
            if os.name=='nt':
                command=['powershell','-NoProfile','-ExecutionPolicy','Bypass','-File',sdk/'Build.ps1']
                if args.ide_root:command+=['-IdeRoot',args.ide_root]
            elif sys.platform=='darwin':command=[sys.executable,sdk/'output/build-macos.py']
            else:raise SystemExit('Build the Smartwatch SDK first and use --skip-sdk-build on this platform')
            run(command)
        sources=sorted((project/'src').glob('*.c'))
        if not (project/'src/main.c').is_file():raise SystemExit('Application src/main.c missing')
        # Current DMA transport uses the HAL-independent command parser too.
        # The application picker's Turkish glyph fonts come with them too.
        keep={'App/platform/watch_display_stm32.o','App/platform/watch_gpu_stm32.o',
              'App/services/watch_led_commands.o','App/views/watch_fonts.o'}
        debug=sdk/'Debug'
        object_file=debug/('objects.list' if os.name=='nt' else 'objects.rsp')
        if not object_file.is_file():raise SystemExit('SDK object list missing; build without --skip-sdk-build first')
        shared=[]
        for line in object_file.read_text().splitlines():
            relative=line.strip().strip('"')
            if not relative:continue
            obj=Path(relative)
            if not obj.is_absolute():obj=debug/obj
            name=obj.relative_to(debug).as_posix()
            if name in ('Core/Src/main.o','Core_Src_main.c.o'):continue
            if name.startswith('App/') and name not in keep:continue
            if name.startswith('App_') and name not in {p[:-2].replace('/','_')+'.c.o' for p in keep}:continue
            if not obj.is_file():raise SystemExit('SDK object missing: '+str(obj))
            shared.append(obj)
        common+=['-O2','-fshort-enums','-DLV_CONF_INCLUDE_SIMPLE','-I'+str(project/'src')]
        includes+=['App','Middlewares','Middlewares/lvgl','Middlewares/NemaGFX/include',
                   'Drivers/BSP/STM32U5x9J-DK','Drivers/BSP/Components/Common']
        linker=sdk/'STM32U5A9NJHXQ_FLASH.ld'
        libraries=[sdk/'Middlewares/NemaGFX/lib/core/cortex_m33_revC/gcc/libnemagfx-float-abi-hard.a','-lm']
    common+=['-I'+str(sdk/path) for path in includes]
    common+=['-I'+str(sdk.parent/'AppManager'/path) for path in ('include','platform','ui')]
    out=project/'build';out.mkdir(exist_ok=True)
    objects=[]
    for source in sources:
        obj=out/(source.stem+'.o');objects.append(obj)
        language=['-x','assembler-with-cpp'] if source.suffix=='.s' else ['-std=gnu11']
        run([tool('gcc'),*common,*language,'-c',source,'-o',obj])
    if target!='DeviceBoot':objects+=shared
    response=out/'link.rsp'
    response.write_text('\n'.join('"'+str(path).replace('\\','/')+'"' for path in objects)+'\n',encoding='utf-8')
    elf=out/(target+'.elf')
    run([tool('gcc'),*common,'@'+str(response),'-T'+str(linker),
         '-Wl,--gc-sections,-z,noexecstack,-Map='+str(out/(target+'.map')),
         '--specs=nosys.specs','-Wl,--start-group',*libraries,'-lc','-Wl,--end-group','-o',elf])
    firmware=project/'Firmware';firmware.mkdir(exist_ok=True)
    binary=firmware/(target+'.bin')
    run([tool('objcopy'),'-O','binary',elf,binary]);binary.chmod(0o644)
    run([tool('size'),elf])
    print(f'{target}: {binary}',flush=True)
