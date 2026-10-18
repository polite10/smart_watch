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
    u.reg_write(UC_ARM_REG_SP, 0x2026ffe0)  # Below DeviceBoot's retained mailbox.
    u.reg_write(UC_ARM_REG_LR, 0x0fff0001)
    u.emu_start(symbols[name] | 1, 0x0fff0000, count=1000000)
    assert u.reg_read(UC_ARM_REG_PC) == 0x0fff0000 or (name == 'device_runtime_poll' and reset_requested), 'Function did not return: ' + name

checks = []
reset_requested = False

def reset_write(uc, access, address, size, value, data):
    global reset_requested
    if value & 4:
        reset_requested = True
        u.emu_stop()
u.hook_add(UC_HOOK_MEM_WRITE, reset_write, begin=0xe000ed0c, end=0xe000ed0c)
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
RXDMA = symbols['led_demo_uart_rx_dma']
RX_DMA_REG = 0x400200d0
UART_ACK = (1 << 21) | (1 << 22)
TIM_REG, UART_REG, DMA_REG, EXTI_REG, GPIOC, GPIOE = 0x40001000, 0x40013800, 0x40020050, 0x46022000, 0x42020800, 0x42021000
# Simulate TX enable acknowledgement, BSRR output updates and DMA suspension.
write32(reg(UART_REG, 'USART_TypeDef', 'ISR'), UART_ACK)
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
    handle = u.reg_read(UC_ARM_REG_R0)
    base = get(handle, 'DMA_HandleTypeDef', 'Instance')
    put(handle, 'DMA_HandleTypeDef', 'State', 1)  # HAL_DMA_STATE_READY
    put(handle, 'DMA_HandleTypeDef', 'Lock', 0)  # HAL_UNLOCKED, as in HAL_DMA_Abort
    queue = get(handle, 'DMA_HandleTypeDef', 'LinkedListQueue')
    if queue:
        put(queue, 'DMA_QListTypeDef', 'State', 1)
    write32(reg(base, 'DMA_Channel_TypeDef', 'CCR'), 0)
    write32(reg(base, 'DMA_Channel_TypeDef', 'CBR1'), 0)
    ret()
hook('HAL_DMA_Abort', abort_dma)
# Model an immediate abort completion on UART line errors; the real HAL then
# invokes UART_DMAAbortOnError and our error callback in the USART IRQ path.
def abort_dma_it():
    abort_dma()
    ret(1)  # HAL invokes the completion callback directly on this return path.
hook('HAL_DMA_Abort_IT', abort_dma_it)

def load_rx_node(uc, access, address, size, value, data):
    # Hardware fetches this static node when the linked-list channel is enabled.
    if value & 1 and not read32(address) & 1:
        node = symbols['rx_node']
        for offset, name in enumerate(('CTR1', 'CTR2', 'CBR1', 'CSAR', 'CDAR', 'CLLR')):
            write32(reg(RX_DMA_REG, 'DMA_Channel_TypeDef', name), read32(node + offset * 4))
u.hook_add(UC_HOOK_MEM_WRITE, load_rx_node,
           begin=reg(RX_DMA_REG, 'DMA_Channel_TypeDef', 'CCR'),
           end=reg(RX_DMA_REG, 'DMA_Channel_TypeDef', 'CCR'))

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
    write32(reg(UART_REG, 'USART_TypeDef', 'ISR'), UART_ACK | (1 << 6))
    call('USART1_IRQHandler')
    write32(reg(UART_REG, 'USART_TypeDef', 'ISR'), UART_ACK)

call('led_button_demo_init')
# RESET in CCR is self-clearing in hardware.
write32(reg(DMA_REG, 'DMA_Channel_TypeDef', 'CCR'), read32(reg(DMA_REG, 'DMA_Channel_TypeDef', 'CCR')) & ~2)
check(get(TIMER, 'TIM_HandleTypeDef', 'Init.Prescaler') == 15999 and get(TIMER, 'TIM_HandleTypeDef', 'Init.Period') == 99, 'TIM6 is configured for 10 ms interrupts at 160 MHz')
check(read32(reg(EXTI_REG, 'EXTI_TypeDef', 'RTSR1')) & (1 << 13) and read32(reg(EXTI_REG, 'EXTI_TypeDef', 'FTSR1')) & (1 << 13), 'PC13 EXTI detects both press and release edges')
check(get(UART, 'UART_HandleTypeDef', 'Init.BaudRate') == 115200 and get(UART, 'UART_HandleTypeDef', 'Init.Mode') == 12, 'USART1 is configured for 115200 baud, full duplex 8N1')
check(get(DMA, 'DMA_HandleTypeDef', 'Init.Request') == 25 and get(DMA, 'DMA_HandleTypeDef', 'Parent') == UART and get(UART, 'UART_HandleTypeDef', 'hdmatx') == DMA, 'GPDMA channel 0 uses USART1_TX request and linked HAL handles')
check(get(RXDMA, 'DMA_HandleTypeDef', 'Mode') == 0x81 and get(UART, 'UART_HandleTypeDef', 'hdmarx') == RXDMA and get(RXDMA, 'DMA_HandleTypeDef', 'Parent') == UART, 'GPDMA channel 1 is linked for circular USART1 reception')
check(read32(reg(RX_DMA_REG, 'DMA_Channel_TypeDef', 'CSAR')) == reg(UART_REG, 'USART_TypeDef', 'RDR') and read32(reg(RX_DMA_REG, 'DMA_Channel_TypeDef', 'CDAR')) == symbols['rx_dma_buffer'], 'RX linked-list node transfers USART1 RDR into the SRAM buffer')
check(read32(reg(RX_DMA_REG, 'DMA_Channel_TypeDef', 'CBR1')) == 256 and read32(reg(RX_DMA_REG, 'DMA_Channel_TypeDef', 'CCR')) & (1 << 9), 'Circular RX starts with 256 bytes and keeps half-transfer interrupts enabled')
advance(990)
call('led_button_demo_poll')
check(not led_edges and not messages, 'Before 1000 ms there is no LED transition or UART report')
advance(10)
call('led_button_demo_poll')
check(led_edges == [1000], 'LED changes inside TIM6 IRQ at 1000 ms without polling')
check(len(messages) == 1 and 'toggle_ms=1000 cycle_ms=2000 presses=0' in messages[-1][2] and messages[-1][2].endswith('\r\n'), 'The first periodic DMA message reports interval, full cycle and press count')
check(read32(reg(DMA_REG, 'DMA_Channel_TypeDef', 'CSAR')) == messages[-1][1] and read32(reg(DMA_REG, 'DMA_Channel_TypeDef', 'CDAR')) == reg(UART_REG, 'USART_TypeDef', 'TDR'), 'Real HAL DMA points from the static SRAM message to USART1 TDR')
buffer_before = bytes(u.mem_read(symbols['uart_message'], 256))
call('led_button_demo_poll')
check(len(messages) == 1 and bytes(u.mem_read(symbols['uart_message'], 256)) == buffer_before, 'Polling cannot overwrite or restart an active DMA message')
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
check(read32(symbols['uart_errors']) == 2 and messages[-1][2].startswith('ERR rx_data_lost') and read32(reg(RX_DMA_REG, 'DMA_Channel_TypeDef', 'CCR')) & 1, 'A real TX DMA error restarts RX and sends a data-loss notice after main-loop recovery')
complete_dma()
complete_uart()
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
check(read32(symbols['uart_completed']) == 4, 'Successful DMA/UART transmission resumes after both error paths')

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

def rx_irq(flag):
    write32(reg(RX_DMA_REG, 'DMA_Channel_TypeDef', 'CSR'), flag)
    write32(reg(0x40020000, 'DMA_TypeDef', 'MISR'), 2)
    call('GPDMA1_Channel1_IRQHandler')
    write32(reg(0x40020000, 'DMA_TypeDef', 'MISR'), 0)
    write32(reg(RX_DMA_REG, 'DMA_Channel_TypeDef', 'CSR'), 0)
    if flag & (1 << 10):
        # DMA RESET is self-clearing and disables the channel in hardware.
        write32(reg(RX_DMA_REG, 'DMA_Channel_TypeDef', 'CCR'), read32(reg(RX_DMA_REG, 'DMA_Channel_TypeDef', 'CCR')) & ~3)

def rx_idle():
    write32(reg(UART_REG, 'USART_TypeDef', 'ISR'), UART_ACK | (1 << 4))
    call('USART1_IRQHandler')
    write32(reg(UART_REG, 'USART_TypeDef', 'ISR'), UART_ACK)

def receive(data, idle=True):
    # Simulate DMA byte movement and node reload; execute real HT/TC/IDLE IRQs.
    for byte in data:
        remaining = read32(reg(RX_DMA_REG, 'DMA_Channel_TypeDef', 'CBR1')) & 0xffff
        assert 0 < remaining <= 256
        position = 256 - remaining
        u.mem_write(symbols['rx_dma_buffer'] + position, bytes([byte]))
        remaining -= 1
        write32(reg(RX_DMA_REG, 'DMA_Channel_TypeDef', 'CBR1'), remaining or 256)
        if remaining == 128:
            rx_irq(1 << 9)
        elif remaining == 0:
            rx_irq(1 << 8)
    if idle:
        rx_idle()

def drain():
    for _ in range(256):
        call('led_button_demo_poll')
        if read32(symbols['uart_busy']):
            complete_dma()
            complete_uart()
        if (read32(symbols['rx_tail']) == read32(symbols['rx_head']) and
                not read32(symbols['reply_length']) and not read32(symbols['status_pending'])):
            return
    raise AssertionError('Command/reply queue did not drain')

# Error recovery above intentionally discards until a newline.
receive(b'\n')
drain()
before = len(messages)
receive(b'SET INTER')
drain()
check(len(messages) == before, 'An IDLE gap in the middle of a command does not execute it')
receive(b'VAL 750\r\n')
check(read32(symbols['blink_interval_ms']) == 100, 'RX IRQs only queue bytes; they never change LED settings')
drain()
check(read32(symbols['blink_interval_ms']) == 750 and messages[-1][2].startswith('OK LED=LD3 mode=BLINK') and 'toggle_ms=750 cycle_ms=1500' in messages[-1][2], 'A fragmented command applies an arbitrary 750 ms interval and returns the actual state')
previous_edges = len(led_edges)
advance(740)
check(len(led_edges) == previous_edges, 'A serial interval change resets the blink timer to a full interval')
advance(10)
check(len(led_edges) == previous_edges + 1, 'The new serial interval is enforced by TIM6 without polling')
drain()

receive(b'LED ON\n')
call('led_button_demo_poll')
check(read32(symbols['led_on']) == 1 and read32(reg(GPIOE, 'GPIO_TypeDef', 'ODR')) & 1, 'LED ON drives the physical GPIO high on the main loop')
active_message = bytes(u.mem_read(symbols['uart_message'], 256))
receive(b'LED OFF\nHELP\n')
call('led_button_demo_poll')
check(read32(symbols['led_on']) == 0 and read32(symbols['reply_length']) > 0, 'One command reply waits safely while another TX DMA is active')
call('led_button_demo_poll')
check(bytes(u.mem_read(symbols['uart_message'], 256)) == active_message, 'Back-to-back commands cannot overwrite the active TX buffer')
before = len(messages)
drain()
check(len(messages) == before + 2 and messages[-2][2].startswith('OK LED=LD3 mode=OFF') and messages[-1][2].startswith('OK HELP'), 'Queued replies remain ordered and commands wait for reply capacity')
previous_edges = len(led_edges)
advance(2000)
check(len(led_edges) == previous_edges, 'Serial LED OFF remains dark independently of UI polling')
drain()
receive(b'LED ON\n')
drain()
previous_edges = len(led_edges)
advance(2000)
check(len(led_edges) == previous_edges and read32(symbols['led_on']) == 1, 'Serial LED ON stays continuously lit across timer interrupts')
drain()

position = 256 - read32(reg(RX_DMA_REG, 'DMA_Channel_TypeDef', 'CBR1'))
receive(b'\n' * ((252 - position) % 256) + b'LED OFF\r\n')
before = len(messages)
drain()
check(len(messages) == before + 1 and 'mode=OFF state=OFF' in messages[-1][2], 'A command spanning the circular DMA buffer boundary is executed once')
head = read32(symbols['rx_head'])
rx_idle()
call('HAL_UARTEx_RxEventCallback', UART, 128)  # A delayed/stale HT notification.
check(read32(symbols['rx_head']) == head, 'Duplicate IDLE and stale HT notifications cannot replay received bytes')

before = len(messages)
receive(b'LED ON\n' * 80, idle=False)
position = 256 - read32(reg(RX_DMA_REG, 'DMA_Channel_TypeDef', 'CBR1'))
receive(b'\n' * ((128 - position) % 128), idle=False)
drain()
check(sum(m.startswith('OK LED=LD3 mode=ON') for _, _, m in messages[before:]) == 80, 'Continuous RX with HT/TC and no IDLE delivers all 80 commands exactly once')
check(get(UART, 'UART_HandleTypeDef', 'RxState') == 0x22 and read32(reg(RX_DMA_REG, 'DMA_Channel_TypeDef', 'CCR')) & 1, 'Circular DMA reception stays enabled after multiple buffer wraps')

before = len(messages)
receive(b'SET INTERVAL 15\nSET INTERVAL 4294967296\nLED OFF extra\n')
drain()
check([m.split()[1] for _, _, m in messages[before:]] == ['interval_range', 'interval_range', 'invalid_command;'] and read32(symbols['led_mode']) == 1, 'Invalid values, numeric overflow and extra arguments return errors without changing the LED')
receive(b'X' * 80 + b'LED OFF\n')
drain()
check(messages[-1][2].startswith('ERR line_too_long') and read32(symbols['led_mode']) == 1, 'An oversized command cannot execute a valid-looking suffix')
receive(b'RESET PRESSES\nGET STATUS\nLED BLINK\n')
drain()
check(read32(symbols['button_presses']) == 0 and messages[-2][2].startswith('OK LED=LD3 mode=ON') and 'presses=0' in messages[-2][2] and read32(symbols['led_mode']) == 2, 'Reset, explicit status and resuming the retained blink interval work through DMA')

# Saturate the software queue while the main/UI loop is stalled.
receive(b'LED ON\n')
drain()
receive(b'LED OFF\n' + b'\n' * 2200)
check(read32(symbols['rx_overflow_pending']) == 1, 'A stalled main loop detects software RX ring overflow')
drain()
check(read32(symbols['rx_overflows']) == 1 and messages[-1][2].startswith('ERR rx_data_lost') and read32(symbols['led_mode']) == 1, 'Overflow drops queued commands and reports loss without executing a damaged stream')
receive(b'LED OFF\n')
drain()
check(read32(symbols['led_mode']) == 1, 'After data loss, the first damaged line is discarded through its terminator')
receive(b'LED OFF\n')
drain()
check(read32(symbols['led_mode']) == 0, 'A complete command after newline resynchronization is accepted')

receive(b'LED O')
drain()
old_rx_errors = read32(symbols['rx_errors'])
write32(reg(UART_REG, 'USART_TypeDef', 'ISR'), UART_ACK | (1 << 1))  # framing error
call('USART1_IRQHandler')
write32(reg(UART_REG, 'USART_TypeDef', 'ISR'), UART_ACK)
drain()
check(read32(symbols['rx_errors']) == old_rx_errors + 1 and read32(reg(RX_DMA_REG, 'DMA_Channel_TypeDef', 'CCR')) & 1, 'A real UART framing-error IRQ aborts and restarts circular RX DMA')
receive(b'N\nLED ON\n')
drain()
check(read32(symbols['led_mode']) == 1 and messages[-1][2].startswith('OK '), 'RX error recovery rejects the old partial command and accepts a fresh one')
old_rx_errors = read32(symbols['rx_errors'])
rx_irq(1 << 10)  # DMA data transfer error
drain()
check(read32(symbols['rx_errors']) == old_rx_errors + 1 and read32(reg(RX_DMA_REG, 'DMA_Channel_TypeDef', 'CCR')) & 1, 'An RX DMA data-transfer error recovers without stopping the timer or UI loop')
receive(b'\nSET INTERVAL 250\n')
drain()
check(read32(symbols['blink_interval_ms']) == 250 and read32(symbols['led_mode']) == 2, 'Commands work again after an RX DMA error')

receive(b'GET STATUS\n')
call('led_button_demo_poll')
old_rx_errors = read32(symbols['rx_errors'])
advance(250)
call('led_button_demo_poll')
check(read32(symbols['rx_errors']) == old_rx_errors and read32(reg(RX_DMA_REG, 'DMA_Channel_TypeDef', 'CCR')) & 1, 'A TX-only timeout does not abort or restart RX DMA')
drain()
receive(b'LED ON\n')
drain()
edge(False)
advance(40)
edge(True)
advance(40)
check(read32(symbols['mode_index']) == 3 and read32(symbols['led_mode']) == 2 and read32(symbols['blink_interval_ms']) == 250 and read32(symbols['button_presses']) == 1, 'A physical button press after serial control selects the next preset and resumes timer blinking')
telemetry = 0x20260000
call('watch_dma_service_status', telemetry)
check(get(telemetry, 'watch_dma_status_t', 'toggle_ms') == 250 and get(telemetry, 'watch_dma_status_t', 'bytes') == read32(symbols['uart_bytes']), 'LCD DMA telemetry reads the same runtime interval and completed TX byte count')
receive(b'LED OFF\n')
drain()
call('watch_dma_service_status', telemetry)
check(get(telemetry, 'watch_dma_status_t', 'toggle_ms') == 0 and get(telemetry, 'watch_dma_status_t', 'led_on') == 0, 'LCD telemetry immediately reflects serial LED OFF instead of the old button preset')

# Device Studio's existing REQ/JSON protocol must share DMA without RXNE theft.
app_name = 0x20260100
u.mem_write(app_name, b'Smartwatch\0')
call('device_runtime_init', app_name, 0)
check(not read32(reg(UART_REG, 'USART_TypeDef', 'CR1')) & (1 << 5), 'Device runtime initialization does not enable a competing RXNE interrupt reader')
before = len(messages)
receive(b'REQ 7 HE')
drain()
check(len(messages) == before, 'A fragmented Device Studio request waits for the line ending')
receive(b'LLO\nLED ON\nREQ 8 STATUS\n')
drain()
replies = [m for _, _, m in messages[before:]]
hello, status = json.loads(replies[0]), json.loads(replies[2])
check(len(replies) == 3 and hello['seq'] == 7 and hello['app'] == 'Smartwatch' and hello['protocol'] == 1 and hello['boot'] == 0 and hello['error'] == '' and replies[1].startswith('OK LED=') and status['seq'] == 8, 'Device JSON and LED replies share one ordered TX DMA path without spurious errors')
receive(b'REQ 9 BOOT\n')
drain()
call('device_runtime_poll')
check(json.loads(messages[-1][2])['error'] == 'bootloader_missing' and not reset_requested, 'BOOT with no installed bootloader reports the existing JSON error and cannot reset')
receive(b'REQ 10 UNKNOWN\n')
drain()
check(json.loads(messages[-1][2])['error'] == 'unsupported_command', 'Unsupported Device Studio commands keep the existing JSON error contract')
boot_info = 0x08007f00
u.mem_write(boot_info, struct.pack('<8I', 0x44425631, 1, 0x0800a000, 0x3f2000, 1, 0, 0, 0))
receive(b'HELP\n')
call('led_button_demo_poll')
receive(b'REQ 11 BOOT\n')
call('led_button_demo_poll')
complete_dma()
complete_uart()  # This is the HELP reply, not BOOT's reply.
advance(30)
call('device_runtime_poll')
check(not reset_requested, 'Completing an earlier reply cannot arm a queued BOOT request')
call('led_button_demo_poll')
check(json.loads(messages[-1][2])['seq'] == 11 and json.loads(messages[-1][2])['error'] == '' and not reset_requested, 'A valid BOOT request starts its DMA reply before any reset is armed')
# Even an elapsed delay and DMA TC are insufficient without the UART wire TC.
advance(30)
complete_dma()
call('device_runtime_poll')
check(not reset_requested and read32(0x2026fff0) == 0, 'DMA completion alone cannot trigger a boot reset before the UART reply finishes')
complete_uart()
advance(10)
call('device_runtime_poll')
check(not reset_requested, 'BOOT retains the 20 ms USB delivery grace after its own UART TC')
advance(10)
call('device_runtime_poll')
check(reset_requested and read32(0x2026fff0) == 0xb00710ad and read32(0x2026fff4) == (~0xb00710ad & 0xffffffff), 'After the matching reply and grace period, BOOT writes the retained mailbox and requests reset')
for name in ('button_presses', 'uart_completed', 'uart_errors', 'rx_errors', 'rx_overflows'):
    write32(symbols[name], 0xffffffff)
tick = 0xffffffff
receive(b'GET STATUS\n')
drain()
check(messages[-1][2].startswith('OK LED=') and messages[-1][2].endswith('\r\n') and len(messages[-1][2]) < 256, 'The status buffer fits maximum-width counters without truncation')
report = {'passed': sum(ok for ok, _ in checks), 'failed': sum(not ok for ok, _ in checks),
          'checks': checks, 'elf_sha256': hashlib.sha256((ROOT / 'Debug/Smartwatch.elf').read_bytes()).hexdigest(),
          'scope': 'Real compiled ARM command parser/demo and EXTI/TIM6/GPDMA TX/RX/UART HAL IRQ paths. MMIO, DMA byte movement, linked-list node fetch/reload, UART acknowledgement and DMA abort completion are simulated; no physical board timing or UART wire data is measured.'}
(ROOT / 'output/verify-led-button.json').write_text(json.dumps(report, indent=2) + '\n')
raise SystemExit(bool(report['failed']))
