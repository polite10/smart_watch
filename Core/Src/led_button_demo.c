/* B1/EXTI and transport-independent commands control LD3; USART1 RX/TX use DMA. */
#include "led_button_demo.h"
#include "smartwatch.h"
#include "stm32u5x9j_discovery.h"
#include "services/watch_dma_service.h"
#include "services/watch_led_commands.h"
#include "device_runtime.h"
#include <stdio.h>
#include <string.h>

#define TIMER_STEP_MS       10U
#define DEBOUNCE_MS         40U
#define STATUS_INTERVAL_MS  1000U
#define UART_TIMEOUT_MS     250U
#define CONTROL_IRQ_PRIO    6U
#define UART_IRQ_PRIO       7U
#define RX_DMA_SIZE         256U
#define RX_RING_SIZE        2048U
#define UART_MESSAGE_SIZE   256U

TIM_HandleTypeDef led_demo_timer;
UART_HandleTypeDef led_demo_uart;
DMA_HandleTypeDef led_demo_uart_tx_dma;
DMA_HandleTypeDef led_demo_uart_rx_dma;

/* Slow down to OFF, then restart at the fastest speed. Start at 1000 ms. */
static const uint32_t blink_intervals_ms[] = {1000U, 0U, 100U, 250U, 500U};
static volatile uint32_t mode_index;
static volatile uint32_t button_presses;
static volatile uint32_t led_on;
enum { LED_MODE_OFF, LED_MODE_ON, LED_MODE_BLINK };
static volatile uint32_t led_mode = LED_MODE_BLINK;
static volatile uint32_t blink_interval_ms = 1000U;
static volatile uint32_t last_button_edge;
static volatile uint32_t debounce_pending;
static volatile uint32_t status_pending;
static volatile uint32_t uart_busy;
static volatile uint32_t uart_error_pending;
static volatile uint32_t uart_completed;
static volatile uint32_t uart_bytes,uart_message_bytes;
static uint32_t uart_errors;
static uint32_t stable_button;
static uint32_t blink_elapsed_ms;
static uint32_t status_elapsed_ms;
static uint32_t uart_started_at;
/* DMA owns this SRAM buffer until UART TC (or a completed abort). */
static char uart_message[UART_MESSAGE_SIZE];
static char command_reply[UART_MESSAGE_SIZE];
static size_t reply_length;
static bool reply_boot, uart_boot;
/* A static self-linked node keeps RX running across IDLE/HT/TC events. */
static DMA_NodeTypeDef rx_node __attribute__((aligned(32)));
static DMA_QListTypeDef rx_list;
static uint8_t rx_dma_buffer[RX_DMA_SIZE] __attribute__((aligned(16)));
static uint8_t rx_ring[RX_RING_SIZE];
static volatile uint32_t rx_head, rx_tail, rx_overflow_pending, rx_error_pending;
static uint16_t rx_dma_position;
static uint32_t rx_errors, rx_overflows;
static bool rx_loss_notice;
static watch_led_commands_t command_parser;

static HAL_StatusTypeDef uart_receive_start(void)
{
    rx_dma_position = 0;
    return HAL_UARTEx_ReceiveToIdle_DMA(&led_demo_uart, rx_dma_buffer, sizeof rx_dma_buffer);
}

static void uart_dma_init(void)
{
    RCC_PeriphCLKInitTypeDef clock = {0};
    GPIO_InitTypeDef gpio = {0};

    clock.PeriphClockSelection = RCC_PERIPHCLK_USART1;
    clock.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK2;
    if(HAL_RCCEx_PeriphCLKConfig(&clock) != HAL_OK) Error_Handler();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_GPDMA1_CLK_ENABLE();
    /* PA9 TX / PA10 RX connect USART1 to the ST-LINK virtual COM port. */
    gpio.Pin = GPIO_PIN_9 | GPIO_PIN_10;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    gpio.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(GPIOA, &gpio);

    led_demo_uart_tx_dma.Instance = GPDMA1_Channel0;
    led_demo_uart_tx_dma.Init.Request = GPDMA1_REQUEST_USART1_TX;
    led_demo_uart_tx_dma.Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;
    led_demo_uart_tx_dma.Init.Direction = DMA_MEMORY_TO_PERIPH;
    led_demo_uart_tx_dma.Init.SrcInc = DMA_SINC_INCREMENTED;
    led_demo_uart_tx_dma.Init.DestInc = DMA_DINC_FIXED;
    led_demo_uart_tx_dma.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_BYTE;
    led_demo_uart_tx_dma.Init.DestDataWidth = DMA_DEST_DATAWIDTH_BYTE;
    led_demo_uart_tx_dma.Init.Priority = DMA_LOW_PRIORITY_LOW_WEIGHT;
    led_demo_uart_tx_dma.Init.SrcBurstLength = 1;
    led_demo_uart_tx_dma.Init.DestBurstLength = 1;
    led_demo_uart_tx_dma.Init.TransferAllocatedPort = DMA_SRC_ALLOCATED_PORT0 | DMA_DEST_ALLOCATED_PORT0;
    led_demo_uart_tx_dma.Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
    led_demo_uart_tx_dma.Init.Mode = DMA_NORMAL;
    if(HAL_DMA_Init(&led_demo_uart_tx_dma) != HAL_OK ||
       HAL_DMA_ConfigChannelAttributes(&led_demo_uart_tx_dma, DMA_CHANNEL_NPRIV) != HAL_OK) Error_Handler();

    DMA_NodeConfTypeDef node = {0};
    node.NodeType = DMA_GPDMA_LINEAR_NODE;
    node.Init = led_demo_uart_tx_dma.Init;
    node.Init.Request = GPDMA1_REQUEST_USART1_RX;
    node.Init.Direction = DMA_PERIPH_TO_MEMORY;
    node.Init.SrcInc = DMA_SINC_FIXED;
    node.Init.DestInc = DMA_DINC_INCREMENTED;
    node.TriggerConfig.TriggerPolarity = DMA_TRIG_POLARITY_MASKED;
    node.DataHandlingConfig.DataExchange = DMA_EXCHANGE_NONE;
    node.DataHandlingConfig.DataAlignment = DMA_DATA_RIGHTALIGN_ZEROPADDED;
    if(HAL_DMAEx_List_BuildNode(&node, &rx_node) != HAL_OK ||
       HAL_DMAEx_List_InsertNode_Tail(&rx_list, &rx_node) != HAL_OK ||
       HAL_DMAEx_List_SetCircularMode(&rx_list) != HAL_OK) Error_Handler();
    led_demo_uart_rx_dma.Instance = GPDMA1_Channel1;
    led_demo_uart_rx_dma.InitLinkedList.Priority = DMA_LOW_PRIORITY_LOW_WEIGHT;
    led_demo_uart_rx_dma.InitLinkedList.LinkStepMode = DMA_LSM_FULL_EXECUTION;
    led_demo_uart_rx_dma.InitLinkedList.LinkAllocatedPort = DMA_LINK_ALLOCATED_PORT0;
    led_demo_uart_rx_dma.InitLinkedList.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
    led_demo_uart_rx_dma.InitLinkedList.LinkedListMode = DMA_LINKEDLIST_CIRCULAR;
    if(HAL_DMAEx_List_Init(&led_demo_uart_rx_dma) != HAL_OK ||
       HAL_DMAEx_List_LinkQ(&led_demo_uart_rx_dma, &rx_list) != HAL_OK ||
       HAL_DMA_ConfigChannelAttributes(&led_demo_uart_rx_dma, DMA_CHANNEL_NPRIV) != HAL_OK) Error_Handler();

    led_demo_uart.Instance = USART1;
    led_demo_uart.Init.BaudRate = 115200;
    led_demo_uart.Init.WordLength = UART_WORDLENGTH_8B;
    led_demo_uart.Init.StopBits = UART_STOPBITS_1;
    led_demo_uart.Init.Parity = UART_PARITY_NONE;
    led_demo_uart.Init.Mode = UART_MODE_TX_RX;
    led_demo_uart.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    led_demo_uart.Init.OverSampling = UART_OVERSAMPLING_16;
    led_demo_uart.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    led_demo_uart.Init.ClockPrescaler = UART_PRESCALER_DIV1;
    __HAL_LINKDMA(&led_demo_uart, hdmatx, led_demo_uart_tx_dma);
    __HAL_LINKDMA(&led_demo_uart, hdmarx, led_demo_uart_rx_dma);
    if(HAL_UART_Init(&led_demo_uart) != HAL_OK) Error_Handler();

    HAL_NVIC_SetPriority(GPDMA1_Channel0_IRQn, UART_IRQ_PRIO, 0);
    HAL_NVIC_EnableIRQ(GPDMA1_Channel0_IRQn);
    /* RX DMA and USART IDLE callbacks have equal priority: no nested producer. */
    HAL_NVIC_SetPriority(GPDMA1_Channel1_IRQn, UART_IRQ_PRIO, 0);
    HAL_NVIC_EnableIRQ(GPDMA1_Channel1_IRQn);
    /* Normal DMA completion enables UART TC: USART1 IRQ must also run. */
    HAL_NVIC_SetPriority(USART1_IRQn, UART_IRQ_PRIO, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
    if(uart_receive_start() != HAL_OK) Error_Handler();
}

static void timer_init(void)
{
    RCC_ClkInitTypeDef clock = {0};
    uint32_t flash_latency;
    uint32_t timer_clock = HAL_RCC_GetPCLK1Freq();
    HAL_RCC_GetClockConfig(&clock, &flash_latency);
    if(clock.APB1CLKDivider != RCC_HCLK_DIV1) timer_clock *= 2U;
    /* 10 kHz counter, 100 counts/update = one IRQ every 10 ms.
     * With this project's 160 MHz APB1: PSC=15999, ARR=99. */
    if(timer_clock < 10000U || timer_clock % 10000U || timer_clock / 10000U > 65536U) Error_Handler();
    __HAL_RCC_TIM6_CLK_ENABLE();
    led_demo_timer.Instance = TIM6;
    led_demo_timer.Init.Prescaler = timer_clock / 10000U - 1U;
    led_demo_timer.Init.CounterMode = TIM_COUNTERMODE_UP;
    led_demo_timer.Init.Period = TIMER_STEP_MS * 10U - 1U;
    led_demo_timer.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if(HAL_TIM_Base_Init(&led_demo_timer) != HAL_OK) Error_Handler();
    /* Base_Init generates an update event; discard it before starting. */
    __HAL_TIM_CLEAR_FLAG(&led_demo_timer, TIM_FLAG_UPDATE);
    HAL_NVIC_SetPriority(TIM6_IRQn, CONTROL_IRQ_PRIO, 0);
    HAL_NVIC_EnableIRQ(TIM6_IRQn);
    if(HAL_TIM_Base_Start_IT(&led_demo_timer) != HAL_OK) Error_Handler();
}

void led_button_demo_init(void)
{
    GPIO_InitTypeDef gpio = {0};
    if(BSP_LED_Init(LED_GREEN) != BSP_ERROR_NONE ||
       BSP_PB_Init(BUTTON_USER, BUTTON_MODE_EXTI) != BSP_ERROR_NONE) Error_Handler();

    /* Observe release too: a long hold is one press, release bounce cannot
     * re-arm the button until the level has been stable for 40 ms. */
    HAL_NVIC_DisableIRQ(EXTI13_IRQn);
    gpio.Pin = BUTTON_USER_PIN;
    gpio.Mode = GPIO_MODE_IT_RISING_FALLING;
    gpio.Pull = GPIO_PULLDOWN;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(BUTTON_USER_GPIO_PORT, &gpio);
    stable_button = (uint32_t)BSP_PB_GetState(BUTTON_USER);
    __HAL_GPIO_EXTI_CLEAR_RISING_IT(BUTTON_USER_PIN);
    __HAL_GPIO_EXTI_CLEAR_FALLING_IT(BUTTON_USER_PIN);
    /* Equal priority prevents EXTI/TIM6 from changing debounce state together. */
    HAL_NVIC_SetPriority(EXTI13_IRQn, CONTROL_IRQ_PRIO, 0);
    HAL_NVIC_EnableIRQ(EXTI13_IRQn);

    uart_dma_init();
    timer_init();
}

void BSP_PB_Callback(Button_TypeDef button)
{
    if(button != BUTTON_USER) return;
    last_button_edge = HAL_GetTick();
    debounce_pending = 1;
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *timer)
{
    if(timer != &led_demo_timer) return;
    uint32_t speed_changed = 0;
    if(debounce_pending && HAL_GetTick() - last_button_edge >= DEBOUNCE_MS) {
        uint32_t button = (uint32_t)BSP_PB_GetState(BUTTON_USER);
        debounce_pending = 0;
        if(button != stable_button) {
            stable_button = button;
            if(button == BUTTON_PRESSED) {
                mode_index = (mode_index + 1U) % (sizeof(blink_intervals_ms) / sizeof(blink_intervals_ms[0]));
                ++button_presses;
                blink_elapsed_ms = 0;
                speed_changed = 1;
                uint32_t preset = blink_intervals_ms[mode_index];
                led_mode = preset ? LED_MODE_BLINK : LED_MODE_OFF;
                if(preset) blink_interval_ms = preset;
                else {
                    BSP_LED_Off(LED_GREEN);
                    led_on = 0;
                }
            }
        }
    }
    /* Blink edges remain timer-driven; ON/OFF commands apply atomically below. */
    uint32_t interval = blink_interval_ms;
    if(led_mode == LED_MODE_BLINK) {
        if(!speed_changed) blink_elapsed_ms += TIMER_STEP_MS;
        if(blink_elapsed_ms >= interval) {
            blink_elapsed_ms = 0;
            BSP_LED_Toggle(LED_GREEN);
            led_on ^= 1U;
        }
    }
    status_elapsed_ms += TIMER_STEP_MS;
    if(status_elapsed_ms >= STATUS_INTERVAL_MS) {
        status_elapsed_ms = 0;
        status_pending = 1;
    }
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *uart)
{
    if(uart != &led_demo_uart) return;
    ++uart_completed;
    uart_bytes+=uart_message_bytes;
    if(uart_boot) device_runtime_boot_reply_complete();
    uart_boot = false;
    uart_busy = 0;
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *uart)
{
    if(uart != &led_demo_uart) return;
    /* HAL's DMA error path can stop BOTH directions. Recover them on the loop. */
    uart_error_pending = 1;
    rx_error_pending = 1;
}

static void receive_bytes(const uint8_t *data, uint16_t size)
{
    if(rx_overflow_pending || rx_error_pending) return;
    for(uint16_t i = 0; i < size; ++i) {
        uint32_t next = (rx_head + 1U) % RX_RING_SIZE;
        if(next == rx_tail) { rx_overflow_pending = 1; return; }
        rx_ring[rx_head] = data[i];
        __DMB();
        rx_head = next;
    }
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *uart, uint16_t size)
{
    if(uart != &led_demo_uart || !size || size > RX_DMA_SIZE) return;
    /* Read the live DMA position: a pending HT/TC interrupt can be serviced
     * after IDLE has already delivered newer bytes. Size alone would replay
     * stale data. IDLE is a delivery hint, never a command boundary. */
    uint16_t position = RX_DMA_SIZE - __HAL_DMA_GET_COUNTER(&led_demo_uart_rx_dma);
    if(position == RX_DMA_SIZE) position = 0;
    if(position == rx_dma_position) return;
    __DMB();
    if(position < rx_dma_position) {
        receive_bytes(rx_dma_buffer + rx_dma_position, RX_DMA_SIZE - rx_dma_position);
        rx_dma_position = 0;
    }
    receive_bytes(rx_dma_buffer + rx_dma_position, position - rx_dma_position);
    rx_dma_position = position;
}

static int format_status(char *buffer, size_t capacity, const char *prefix)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    uint32_t mode = led_mode;
    uint32_t interval = mode == LED_MODE_BLINK ? blink_interval_ms : 0U;
    uint32_t configured = blink_interval_ms;
    uint32_t presses = button_presses, state = led_on, completed = uart_completed;
    __set_PRIMASK(primask);
    return snprintf(buffer, capacity,
        "%sLED=LD3 mode=%s state=%s toggle_ms=%lu cycle_ms=%lu presses=%lu uptime_ms=%lu uart_ok=%lu uart_errors=%lu interval_ms=%lu rx_errors=%lu rx_overflows=%lu\r\n",
        prefix, mode == LED_MODE_BLINK ? "BLINK" : mode == LED_MODE_ON ? "ON" : "OFF",
        state ? "ON" : "OFF", (unsigned long)interval, (unsigned long)(interval * 2U),
        (unsigned long)presses, (unsigned long)HAL_GetTick(), (unsigned long)completed,
        (unsigned long)uart_errors, (unsigned long)configured, (unsigned long)rx_errors,
        (unsigned long)rx_overflows);
}

static void execute_command(const watch_led_command_t *command)
{
    reply_boot = false;
    if(command->kind == WATCH_LED_CMD_DEVICE_REQUEST) {
        int length = device_runtime_format_reply(command->sequence, command->device_command,
            command_reply, sizeof command_reply, &reply_boot);
        if(length > 0 && (size_t)length < sizeof command_reply) reply_length = (size_t)length;
        return;
    }
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    switch(command->kind) {
    case WATCH_LED_CMD_ON:
        led_mode = LED_MODE_ON; led_on = 1; BSP_LED_On(LED_GREEN); break;
    case WATCH_LED_CMD_OFF:
        led_mode = LED_MODE_OFF; led_on = 0; BSP_LED_Off(LED_GREEN); break;
    case WATCH_LED_CMD_INTERVAL:
        blink_interval_ms = command->interval_ms;
        /* SET INTERVAL also starts blinking, with a full first interval. */
        /* fall through */
    case WATCH_LED_CMD_BLINK:
        led_mode = LED_MODE_BLINK; blink_elapsed_ms = 0; break;
    case WATCH_LED_CMD_RESET_PRESSES: button_presses = 0; break;
    default: break;
    }
    __set_PRIMASK(primask);
    int length;
    if(command->kind == WATCH_LED_CMD_HELP)
        length = snprintf(command_reply, sizeof command_reply,
            "OK HELP: LED ON|OFF|BLINK; SET INTERVAL <10..60000 ms, step 10>; GET STATUS; RESET PRESSES; HELP\r\n");
    else length = format_status(command_reply, sizeof command_reply, "OK ");
    if(length > 0 && (size_t)length < sizeof command_reply) reply_length = (size_t)length;
}

static void command_poll(void)
{
    if(reply_length) return; /* Backpressure: never execute a command without reply space. */
    if(rx_loss_notice) {
        reply_boot = false;
        const char *error = "ERR rx_data_lost; send a newline, then retry\r\n";
        strcpy(command_reply, error); reply_length = strlen(error); rx_loss_notice = false;
        return;
    }
    for(unsigned budget = 0; budget < RX_DMA_SIZE && rx_tail != rx_head; ++budget) {
        if(rx_error_pending || rx_overflow_pending) return;
        __DMB();
        uint8_t byte = rx_ring[rx_tail];
        __DMB();
        rx_tail = (rx_tail + 1U) % RX_RING_SIZE;
        watch_led_command_t command = {0};
        watch_led_input_result_t result = watch_led_commands_push(&command_parser, byte, &command);
        if(result == WATCH_LED_INPUT_PENDING) continue;
        if(result == WATCH_LED_INPUT_COMMAND) execute_command(&command);
        else {
            reply_boot = false;
            const char *error = result == WATCH_LED_INPUT_RANGE ? "ERR interval_range 10..60000 step 10\r\n" :
                result == WATCH_LED_INPUT_TOO_LONG ? "ERR line_too_long max=63\r\n" : "ERR invalid_command; type HELP\r\n";
            strcpy(command_reply, error); reply_length = strlen(error);
        }
        return;
    }
}

static void receive_recover(void)
{
    if(!rx_error_pending && !rx_overflow_pending) return;
    /* Prevent a callback from publishing bytes across the queue/parser reset. */
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    bool restart = rx_error_pending != 0;
    if(rx_overflow_pending) ++rx_overflows;
    rx_tail = rx_head;
    rx_overflow_pending = 0;
    watch_led_commands_reset(&command_parser, true);
    rx_loss_notice = true;
    __set_PRIMASK(primask);
    if(restart) {
        ++rx_errors;
        /* Abort only RX; a TX timeout alone does not interrupt command reception. */
        if(HAL_UART_AbortReceive(&led_demo_uart) != HAL_OK) Error_Handler();
        /* A failed asynchronous abort may already have cleared USART DMAR.
         * In that case AbortReceive cannot see the still-busy DMA channel. */
        if(HAL_DMA_GetState(&led_demo_uart_rx_dma) != HAL_DMA_STATE_READY &&
           HAL_DMA_Abort(&led_demo_uart_rx_dma) != HAL_OK) Error_Handler();
        __HAL_UART_CLEAR_FLAG(&led_demo_uart, UART_CLEAR_OREF | UART_CLEAR_NEF | UART_CLEAR_FEF | UART_CLEAR_PEF);
        __HAL_UART_SEND_REQ(&led_demo_uart, UART_RXDATA_FLUSH_REQUEST);
        rx_error_pending = 0;
        if(uart_receive_start() != HAL_OK) rx_error_pending = 1;
    }
}

void led_button_demo_poll(void)
{
    if(uart_error_pending || (uart_busy && HAL_GetTick() - uart_started_at >= UART_TIMEOUT_MS)) {
        /* Stop DMA before releasing its buffer, including failed DMA starts. */
        if(HAL_UART_AbortTransmit(&led_demo_uart) != HAL_OK) Error_Handler();
        uart_error_pending = 0;
        uart_busy = 0;
        uart_boot = false;
        ++uart_errors;
    }
    receive_recover();
    command_poll();
    if(uart_busy || (!reply_length && !status_pending)) return;
    int length;
    if(reply_length) {
        length = (int)reply_length;
        memcpy(uart_message, command_reply, reply_length);
        reply_length = 0;
        uart_boot = reply_boot;
        reply_boot = false;
    } else {
        uart_boot = false;
        status_pending = 0;
        length = format_status(uart_message, sizeof uart_message, "");
    }
    if(length <= 0 || (size_t)length >= sizeof(uart_message)) {
        ++uart_errors;
        return;
    }
    uart_started_at = HAL_GetTick();
    uart_message_bytes=(uint32_t)length;
    uart_busy = 1;
    __DMB();
    if(HAL_UART_Transmit_DMA(&led_demo_uart, (const uint8_t *)uart_message, (uint16_t)length) != HAL_OK)
        uart_error_pending = 1;
}

watch_dma_status_t watch_dma_service_status(void)
{
    uint32_t primask=__get_PRIMASK();
    __disable_irq();
    watch_dma_status_t status={uart_completed,uart_bytes,uart_errors,uart_message_bytes,
        led_mode == LED_MODE_BLINK ? blink_interval_ms : 0U,led_on!=0,uart_busy!=0};
    __set_PRIMASK(primask);
    return status;
}
