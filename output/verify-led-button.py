"""Run the built ARM demo and HAL IRQ paths with simulated peripheral registers.

Requires unicorn and pyelftools (also looks in tmp/simulator-libs).
No board is programmed. UART wire timing and physical button bounce are simulated.
"""
import hashlib
import json
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tmp/simulator-libs'))
from unicorn import Uc, UC_ARCH_ARM, UC_MODE_THUMB, UC_MODE_MCLASS, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn.arm_const import UC_CPU_ARM_CORTEX_M33, UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2, UC_ARM_REG_R3, UC_ARM_REG_SP, UC_ARM_REG_LR, UC_ARM_REG_PC
from elftools.elf.elffile import ELFFile

u = Uc(UC_ARCH_ARM, UC_MODE_THUMB | UC_MODE_MCLASS)
u.ctl_set_cpu_model(UC_CPU_ARM_CORTEX_M33)
for address, size in [(0x08000000, 0x400000), (0x20000000, 0x280000),
                      (0x40000000, 0x30000), (0x42020000, 0x4000),
                      (0x46020000, 0x4000), (0xe000e000, 0x2000), (0x0fff0000, 0x1000)]:
    u.mem_map(address, size)
with (ROOT / 'Debug/Smartwatch.elf').open('rb') as source:
    elf = ELFFile(source)
    symbols = {s.name: s['st_value'] for s in elf.get_section_by_name('.symtab').iter_symbols() if s.name}
    for segment in elf.iter_segments():
        if segment['p_type'] == 'PT_LOAD':
            u.mem_write(segment['p_vaddr'], segment.data())
    types = {}
    for cu in elf.get_dwarf_info().iter_CUs():
        for die in cu.iter_DIEs():
            if die.tag in ('DW_TAG_typedef', 'DW_TAG_structure_type') and 'DW_AT_name' in die.attributes:
                types.setdefault(die.attributes['DW_AT_name'].value.decode(), die)

    def resolve(die):
        while die.tag in ('DW_TAG_typedef', 'DW_TAG_const_type', 'DW_TAG_volatile_type'):
            die = die.get_DIE_from_attribute('DW_AT_type')
        return die

    def field(type_name, path):
        die, position = resolve(types[type_name]), 0
        for name in path.split('.'):
            member = next(c for c in die.iter_children() if c.attributes.get('DW_AT_name') and c.attributes['DW_AT_name'].value.decode() == name)
            position += member.attributes['DW_AT_data_member_location'].value
            die = resolve(member.get_DIE_from_attribute('DW_AT_type'))
        return position, die.attributes['DW_AT_byte_size'].value

def read32(address):
    return struct.unpack('<I', u.mem_read(address, 4))[0]

def write32(address, value):
    u.mem_write(address, struct.pack('<I', value & 0xffffffff))

def get(handle, type_name, path):
    position, size = field(type_name, path)
    return int.from_bytes(u.mem_read(handle + position, size), 'little')

def put(handle, type_name, path, value):
    position, size = field(type_name, path)
    u.mem_write(handle + position, value.to_bytes(size, 'little'))

def reg(base, type_name, name):
    return base + field(type_name, name)[0]

def ret(value=0):
    u.reg_write(UC_ARM_REG_R0, value)
    u.reg_write(UC_ARM_REG_PC, u.reg_read(UC_ARM_REG_LR))

def hook(name, action):
    address = symbols[name] & ~1
    u.hook_add(UC_HOOK_CODE, lambda uc, addr, size, data: action(), begin=address, end=address)

def call(name, *args):
    for register, value in zip((UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2, UC_ARM_REG_R3), args):
        u.reg_write(register, value)
    u.reg_write(UC_ARM_REG_SP, 0x20270000)
    u.reg_write(UC_ARM_REG_LR, 0x0fff0001)
    u.emu_start(symbols[name] | 1, 0x0fff0000, count=1000000)
    assert u.reg_read(UC_ARM_REG_PC) == 0x0fff0000, 'Function did not return: ' + name

checks = []
def check(condition, description):
    checks.append((bool(condition), description))
    print(('PASS: ' if condition else 'FAIL: ') + description, flush=True)

def fatal():
    raise AssertionError('Firmware reached Error_Handler')

tick = 0
hook('HAL_GetTick', lambda: ret(tick))
hook('HAL_RCC_GetSysClockFreq', lambda: ret(160000000))
hook('Error_Handler', fatal)
write32(symbols['SystemCoreClock'], 160000000)
TIMER, UART, DMA = (symbols[n] for n in ('led_demo_timer', 'led_demo_uart', 'led_demo_uart_tx_dma'))
TIM_REG, UART_REG, DMA_REG, EXTI_REG, GPIOC, GPIOE = 0x40001000, 0x40013800, 0x40020050, 0x46022000, 0x42020800, 0x42021000
# Simulate TX enable acknowledgement, BSRR output updates and DMA suspension.
write32(reg(UART_REG, 'USART_TypeDef', 'ISR'), 1 << 21)
led_edges = []
def gpio_write(uc, access, address, size, value, data):
    if address == reg(GPIOE, 'GPIO_TypeDef', 'BRR'):
        value <<= 16
    odr = reg(GPIOE, 'GPIO_TypeDef', 'ODR')
    previous = read32(odr)
    current = (previous | (value & 0xffff)) & ~(value >> 16)
    write32(odr, current)
    if (previous ^ current) & 1:
        led_edges.append(tick)
u.hook_add(UC_HOOK_MEM_WRITE, gpio_write, begin=reg(GPIOE, 'GPIO_TypeDef', 'BSRR'), end=reg(GPIOE, 'GPIO_TypeDef', 'BSRR'))
u.hook_add(UC_HOOK_MEM_WRITE, gpio_write, begin=reg(GPIOE, 'GPIO_TypeDef', 'BRR'), end=reg(GPIOE, 'GPIO_TypeDef', 'BRR'))

def abort_dma():
    put(DMA, 'DMA_HandleTypeDef', 'State', 1)  # HAL_DMA_STATE_READY
    put(DMA, 'DMA_HandleTypeDef', 'Lock', 0)  # HAL_UNLOCKED, as in HAL_DMA_Abort
    write32(reg(DMA_REG, 'DMA_Channel_TypeDef', 'CCR'), 0)
    write32(reg(DMA_REG, 'DMA_Channel_TypeDef', 'CBR1'), 0)
    ret()
hook('HAL_DMA_Abort', abort_dma)

messages = []
def observe_tx():
    address, size = u.reg_read(UC_ARM_REG_R1), u.reg_read(UC_ARM_REG_R2)
    messages.append((tick, address, bytes(u.mem_read(address, size)).decode()))
hook('HAL_UART_Transmit_DMA', observe_tx)  # Observe; execute the real HAL function.
fail_dma_start = False
hook('HAL_DMA_Start_IT', lambda: ret(1) if fail_dma_start else None)

def advance(milliseconds, poll=False):
    global tick
    assert milliseconds % 10 == 0
    for _ in range(milliseconds // 10):
        tick = (tick + 10) & 0xffffffff
        write32(reg(TIM_REG, 'TIM_TypeDef', 'SR'), 1)
        call('TIM6_IRQHandler')
        if poll:
            call('led_button_demo_poll')

def edge(pressed):
    write32(reg(GPIOC, 'GPIO_TypeDef', 'IDR'), (1 << 13) if pressed else 0)
    write32(reg(EXTI_REG, 'EXTI_TypeDef', 'RPR1'), (1 << 13) if pressed else 0)
    write32(reg(EXTI_REG, 'EXTI_TypeDef', 'FPR1'), 0 if pressed else (1 << 13))
    call('EXTI13_IRQHandler')
    write32(reg(EXTI_REG, 'EXTI_TypeDef', 'RPR1'), 0)
    write32(reg(EXTI_REG, 'EXTI_TypeDef', 'FPR1'), 0)

def complete_dma():
    write32(reg(DMA_REG, 'DMA_Channel_TypeDef', 'CBR1'), 0)
    write32(reg(DMA_REG, 'DMA_Channel_TypeDef', 'CSR'), 1 << 8)
    write32(reg(0x40020000, 'DMA_TypeDef', 'MISR'), 1)
    call('GPDMA1_Channel0_IRQHandler')
    write32(reg(0x40020000, 'DMA_TypeDef', 'MISR'), 0)
    write32(reg(DMA_REG, 'DMA_Channel_TypeDef', 'CSR'), 0)
    write32(reg(DMA_REG, 'DMA_Channel_TypeDef', 'CCR'), read32(reg(DMA_REG, 'DMA_Channel_TypeDef', 'CCR')) & ~1)

def complete_uart():
    write32(reg(UART_REG, 'USART_TypeDef', 'ISR'), (1 << 21) | (1 << 6))
    call('USART1_IRQHandler')
    write32(reg(UART_REG, 'USART_TypeDef', 'ISR'), 1 << 21)

call('led_button_demo_init')
# RESET in CCR is self-clearing in hardware.
write32(reg(DMA_REG, 'DMA_Channel_TypeDef', 'CCR'), read32(reg(DMA_REG, 'DMA_Channel_TypeDef', 'CCR')) & ~2)
check(get(TIMER, 'TIM_HandleTypeDef', 'Init.Prescaler') == 15999 and get(TIMER, 'TIM_HandleTypeDef', 'Init.Period') == 99, 'TIM6 is configured for 10 ms interrupts at 160 MHz')
check(read32(reg(EXTI_REG, 'EXTI_TypeDef', 'RTSR1')) & (1 << 13) and read32(reg(EXTI_REG, 'EXTI_TypeDef', 'FTSR1')) & (1 << 13), 'PC13 EXTI detects both press and release edges')
check(get(UART, 'UART_HandleTypeDef', 'Init.BaudRate') == 115200 and get(UART, 'UART_HandleTypeDef', 'Init.Mode') == 8, 'USART1 is configured for 115200 baud, transmit only')
check(get(DMA, 'DMA_HandleTypeDef', 'Init.Request') == 25 and get(DMA, 'DMA_HandleTypeDef', 'Parent') == UART and get(UART, 'UART_HandleTypeDef', 'hdmatx') == DMA, 'GPDMA channel 0 uses USART1_TX request and linked HAL handles')
advance(990)
call('led_button_demo_poll')
check(not led_edges and not messages, 'Before 1000 ms there is no LED transition or UART report')
advance(10)
call('led_button_demo_poll')
check(led_edges == [1000], 'LED changes inside TIM6 IRQ at 1000 ms without polling')
check(len(messages) == 1 and 'toggle_ms=1000 cycle_ms=2000 presses=0' in messages[-1][2] and messages[-1][2].endswith('\r\n'), 'The first periodic DMA message reports interval, full cycle and press count')
check(read32(reg(DMA_REG, 'DMA_Channel_TypeDef', 'CSAR')) == messages[-1][1] and read32(reg(DMA_REG, 'DMA_Channel_TypeDef', 'CDAR')) == reg(UART_REG, 'USART_TypeDef', 'TDR'), 'Real HAL DMA points from the static SRAM message to USART1 TDR')
buffer_before = bytes(u.mem_read(symbols['uart_message'], 192))
call('led_button_demo_poll')
check(len(messages) == 1 and bytes(u.mem_read(symbols['uart_message'], 192)) == buffer_before, 'Polling cannot overwrite or restart an active DMA message')
complete_dma()
check(read32(symbols['uart_busy']) == 1 and read32(symbols['uart_completed']) == 0, 'DMA completion retains the buffer until the final UART TC interrupt')
complete_uart()
check(read32(symbols['uart_busy']) == 0 and read32(symbols['uart_completed']) == 1, 'USART1 TC releases the buffer and counts one completed message')

edge(True)
advance(10)
edge(False)
advance(10)
edge(True)
advance(30)
check(read32(symbols['button_presses']) == 0, 'Contact bounce does not change speed before a stable 40 ms press')
advance(10)
check(read32(symbols['button_presses']) == 1 and read32(symbols['mode_index']) == 1, 'The first debounced press selects permanent OFF exactly once')
check(read32(symbols['led_on']) == 0 and not read32(reg(GPIOE, 'GPIO_TypeDef', 'ODR')) & 1, 'Entering OFF switches off an already lit physical LED immediately')
change_time, previous_edges = tick, len(led_edges)
advance(3000)
check(len(led_edges) == previous_edges and read32(symbols['led_on']) == 0, 'OFF remains dark through 300 timer interrupts without main-loop polling')
check(read32(symbols['button_presses']) == 1 and read32(symbols['mode_index']) == 1, 'Holding the button in OFF counts only one press')
edge(False)
advance(10)
edge(True)
advance(10)
edge(False)
advance(40)
check(read32(symbols['button_presses']) == 1, 'Release bounce cannot produce another press')
for expected_index, interval in [(2, 100), (3, 250), (4, 500), (0, 1000)]:
    edge(True)
    advance(40)
    change_time, previous_edges = tick, len(led_edges)
    check(read32(symbols['mode_index']) == expected_index, f'Next press selects mode index {expected_index}')
    advance(interval - 10)
    check(len(led_edges) == previous_edges, f'LED waits the full {interval} ms interval')
    advance(10)
    check(led_edges[-1] - change_time == interval, f'TIM6 produces the {interval} ms interval')
    edge(False)
    advance(40)

edge(True)
advance(40)
previous_edges = len(led_edges)
edge(False)
advance(2040)
check(read32(symbols['mode_index']) == 1 and len(led_edges) == previous_edges and read32(symbols['led_on']) == 0, 'Slowing through all four blink intervals wraps back to sustained OFF')

# Reports coalesce if the UI has not polled: one current status, no stale burst.
call('led_button_demo_poll')
check(len(messages) == 2 and 'presses=6' in messages[-1][2] and 'uart_ok=1' in messages[-1][2], 'Delayed polling sends one current snapshot with six presses and TX completion count')
check('mode=OFF state=OFF toggle_ms=0 cycle_ms=0' in messages[-1][2], 'UART DMA keeps reporting in OFF and distinguishes disabled mode from a blink off-phase')
complete_dma()
complete_uart()
before = len(messages)
advance(1000, poll=True)
check(len(messages) == before + 1, 'Normal polling schedules exactly one DMA report per second')
advance(250, poll=True)
check(read32(symbols['uart_busy']) == 0 and read32(symbols['uart_errors']) == 1, 'A missing UART completion aborts safely after 250 ms')
advance(1000)
call('led_button_demo_poll')
check('uart_errors=1' in messages[-1][2] and read32(symbols['uart_busy']) == 1 and read32(symbols['uart_error_pending']) == 0, 'UART reporting resumes after a timed out transfer')
write32(reg(DMA_REG, 'DMA_Channel_TypeDef', 'CSR'), 1 << 10)
write32(reg(0x40020000, 'DMA_TypeDef', 'MISR'), 1)
call('GPDMA1_Channel0_IRQHandler')
write32(reg(0x40020000, 'DMA_TypeDef', 'MISR'), 0)
write32(reg(DMA_REG, 'DMA_Channel_TypeDef', 'CSR'), 0)
write32(reg(DMA_REG, 'DMA_Channel_TypeDef', 'CCR'), read32(reg(DMA_REG, 'DMA_Channel_TypeDef', 'CCR')) & ~3)
call('led_button_demo_poll')
check(read32(symbols['uart_busy']) == 0 and read32(symbols['uart_errors']) == 2, 'A real DMA error IRQ releases ownership through main-loop recovery')
fail_dma_start = True
advance(1000)
call('led_button_demo_poll')
call('led_button_demo_poll')
check(read32(symbols['uart_busy']) == 0 and read32(symbols['uart_errors']) == 3, 'An immediate DMA start failure is counted and recovered')
fail_dma_start = False
advance(1000)
call('led_button_demo_poll')
complete_dma()
complete_uart()
check(read32(symbols['uart_completed']) == 3, 'Successful DMA/UART transmission resumes after both error paths')

# The unsigned debounce subtraction must work across the HAL tick rollover.
tick = 0xfffffff0
edge(True)
advance(40)
check(read32(symbols['button_presses']) == 7 and read32(symbols['mode_index']) == 2, 'Button debounce works across the HAL tick rollover and re-enables blinking from OFF')
previous_edges = len(led_edges)
advance(90)
check(len(led_edges) == previous_edges, 'Leaving OFF waits the full new interval before lighting the LED')
advance(10)
check(len(led_edges) == previous_edges + 1 and read32(symbols['led_on']) == 1, 'After OFF, the next press resumes 100 ms blinking in the timer IRQ')
report = {'passed': sum(ok for ok, _ in checks), 'failed': sum(not ok for ok, _ in checks),
          'checks': checks, 'elf_sha256': hashlib.sha256((ROOT / 'Debug/Smartwatch.elf').read_bytes()).hexdigest(),
          'scope': 'Real compiled ARM demo and EXTI/TIM6/GPDMA/UART HAL IRQ paths. MMIO, DMA data movement, TX acknowledgement and DMA abort suspension are simulated; no physical board timing or UART wire data is measured.'}
(ROOT / 'output/verify-led-button.json').write_text(json.dumps(report, indent=2) + '\n')
raise SystemExit(bool(report['failed']))
