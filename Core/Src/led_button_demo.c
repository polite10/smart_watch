/* B1/EXTI -> debounced speed selection; TIM6 -> LD3; USART1/GPDMA -> status. */
#include "led_button_demo.h"
#include "smartwatch.h"
#include "stm32u5x9j_discovery.h"
#include "services/watch_dma_service.h"
#include <stdio.h>

#define TIMER_STEP_MS       10U
#define DEBOUNCE_MS         40U
#define STATUS_INTERVAL_MS  1000U
#define UART_TIMEOUT_MS     250U
#define CONTROL_IRQ_PRIO    6U
#define UART_IRQ_PRIO       7U

TIM_HandleTypeDef led_demo_timer;
UART_HandleTypeDef led_demo_uart;
DMA_HandleTypeDef led_demo_uart_tx_dma;

/* Slow down to OFF, then restart at the fastest speed. Start at 1000 ms. */
static const uint32_t blink_intervals_ms[] = {1000U, 0U, 100U, 250U, 500U};
static volatile uint32_t mode_index;
static volatile uint32_t button_presses;
static volatile uint32_t led_on;
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
static char uart_message[192];

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
    /* PA9/AF7 is wired to the ST-LINK virtual COM port's RX. */
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
    if(HAL_UART_Init(&led_demo_uart) != HAL_OK) Error_Handler();

    HAL_NVIC_SetPriority(GPDMA1_Channel0_IRQn, UART_IRQ_PRIO, 0);
    HAL_NVIC_EnableIRQ(GPDMA1_Channel0_IRQn);
    /* Normal DMA completion enables UART TC: USART1 IRQ must also run. */
    HAL_NVIC_SetPriority(USART1_IRQn, UART_IRQ_PRIO, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
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
                if(blink_intervals_ms[mode_index] == 0U) {
                    BSP_LED_Off(LED_GREEN);
                    led_on = 0;
                }
            }
        }
    }
    /* Only this timer callback changes LD3, independent of UI loop latency. */
    uint32_t interval = blink_intervals_ms[mode_index];
    if(interval != 0U) {
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
    uart_busy = 0;
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *uart)
{
    if(uart == &led_demo_uart) uart_error_pending = 1;
}

void led_button_demo_poll(void)
{
    if(uart_error_pending || (uart_busy && HAL_GetTick() - uart_started_at >= UART_TIMEOUT_MS)) {
        /* Stop DMA before releasing its buffer, including failed DMA starts. */
        if(HAL_UART_AbortTransmit(&led_demo_uart) != HAL_OK) Error_Handler();
        uart_error_pending = 0;
        uart_busy = 0;
        ++uart_errors;
    }
    if(uart_busy || !status_pending) return;

    /* Take one consistent snapshot; formatting and DMA start run with IRQs on. */
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    uint32_t interval = blink_intervals_ms[mode_index];
    uint32_t presses = button_presses;
    uint32_t state = led_on;
    uint32_t completed = uart_completed;
    status_pending = 0;
    __set_PRIMASK(primask);

    int length = snprintf(uart_message, sizeof(uart_message),
        "LED=LD3 mode=%s state=%s toggle_ms=%lu cycle_ms=%lu presses=%lu uptime_ms=%lu uart_ok=%lu uart_errors=%lu\r\n",
        interval ? "BLINK" : "OFF", state ? "ON" : "OFF", (unsigned long)interval, (unsigned long)(interval * 2U),
        (unsigned long)presses, (unsigned long)HAL_GetTick(),
        (unsigned long)completed, (unsigned long)uart_errors);
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
        blink_intervals_ms[mode_index],led_on!=0,uart_busy!=0};
    __set_PRIMASK(primask);
    return status;
}
