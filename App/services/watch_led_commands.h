#ifndef WATCH_LED_COMMANDS_H
#define WATCH_LED_COMMANDS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define WATCH_LED_COMMAND_MAX 63U
#define WATCH_LED_INTERVAL_MIN_MS 10U
#define WATCH_LED_INTERVAL_MAX_MS 60000U

typedef enum {
    WATCH_LED_CMD_ON, WATCH_LED_CMD_OFF, WATCH_LED_CMD_BLINK,
    WATCH_LED_CMD_INTERVAL, WATCH_LED_CMD_STATUS, WATCH_LED_CMD_RESET_PRESSES,
    WATCH_LED_CMD_HELP, WATCH_LED_CMD_DEVICE_REQUEST
} watch_led_command_kind_t;

typedef struct {
    watch_led_command_kind_t kind;
    uint32_t interval_ms;
    uint32_t sequence;
    char device_command[40];
} watch_led_command_t;

typedef enum {
    WATCH_LED_INPUT_PENDING, WATCH_LED_INPUT_COMMAND,
    WATCH_LED_INPUT_INVALID, WATCH_LED_INPUT_TOO_LONG, WATCH_LED_INPUT_RANGE
} watch_led_input_result_t;

typedef struct {
    char line[WATCH_LED_COMMAND_MAX + 1U];
    size_t length;
    watch_led_input_result_t error;
    bool discard;
} watch_led_commands_t;

/* Pure stream parser: no HAL, UART, DMA or LVGL dependency. Call on one thread.
 * Feed bytes from any transport. CR, LF and CRLF terminate a command; blanks
 * are ignored. Output is valid only when WATCH_LED_INPUT_COMMAND is returned. */
watch_led_input_result_t watch_led_commands_push(watch_led_commands_t *parser,
                                                uint8_t byte,
                                                watch_led_command_t *command);
/* After transport data loss, discard through the next line ending so a suffix
 * of a damaged command cannot be executed as a new command. */
void watch_led_commands_reset(watch_led_commands_t *parser, bool discard);

#endif
