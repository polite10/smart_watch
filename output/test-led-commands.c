/* Host-side tests exercise framing/validation without linking HAL or LVGL. */
#include "services/watch_led_commands.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static watch_led_commands_t parser;
static watch_led_command_t command;
static unsigned commands, errors, checks;
static watch_led_input_result_t last;

static void feed(const void *bytes, size_t size)
{
    const uint8_t *data = bytes;
    for(size_t i = 0; i < size; ++i) {
        watch_led_input_result_t result = watch_led_commands_push(&parser, data[i], &command);
        if(result == WATCH_LED_INPUT_PENDING) continue;
        last = result;
        if(result == WATCH_LED_INPUT_COMMAND) ++commands;
        else ++errors;
    }
}

#define FEED(text) feed(text, sizeof(text) - 1U)
#define CHECK(condition) do { assert(condition); ++checks; } while(0)

int main(void)
{
    FEED("SET IN"); CHECK(commands == 0);
    FEED("TERVAL 2"); CHECK(commands == 0);
    FEED("50\r"); CHECK(commands == 1 && command.kind == WATCH_LED_CMD_INTERVAL && command.interval_ms == 250);
    FEED("\n"); CHECK(commands == 1 && errors == 0);
    FEED("LED ON\nLED OFF\rLED BLINK\r\n"); CHECK(commands == 4 && command.kind == WATCH_LED_CMD_BLINK);
    FEED("GET STATUS\n"); CHECK(command.kind == WATCH_LED_CMD_STATUS);
    FEED("RESET PRESSES\n"); CHECK(command.kind == WATCH_LED_CMD_RESET_PRESSES);
    FEED(" \t HELP \t\n"); CHECK(command.kind == WATCH_LED_CMD_HELP);
    FEED(" \t\r\n\n"); CHECK(commands == 7 && errors == 0);
    FEED("SET\tINTERVAL  10\n"); CHECK(command.interval_ms == 10);
    FEED("SET INTERVAL 60000\n"); CHECK(command.interval_ms == 60000);
    const char *invalid[] = {"SET INTERVAL 0\n", "SET INTERVAL 9\n", "SET INTERVAL 15\n",
        "SET INTERVAL 60010\n", "SET INTERVAL 4294967296\n", "SET INTERVAL 999999999999999999999999999999999999\n"};
    for(unsigned i = 0; i < sizeof invalid / sizeof *invalid; ++i) {
        feed(invalid[i], strlen(invalid[i])); CHECK(last == WATCH_LED_INPUT_RANGE);
    }
    const char *malformed[] = {"LED\n", "LED ON extra\n", "HELP extra\n", "GET\n", "GET STATUS extra\n",
        "RESET PRESSES extra\n", "SET INTERVAL -10\n", "SET INTERVAL +10\n", "SET INTERVAL 1e3\n",
        "SET INTERVAL 100x\n", "SET INTERVAL\n", "UNKNOWN\n", "led off\n", "A B C D E\n"};
    for(unsigned i = 0; i < sizeof malformed / sizeof *malformed; ++i) {
        feed(malformed[i], strlen(malformed[i])); CHECK(last == WATCH_LED_INPUT_INVALID);
    }
    unsigned before = commands;
    const uint8_t nul[] = {'L','E','D',' ',0,'O','N','\n'};
    feed(nul, sizeof nul); CHECK(last == WATCH_LED_INPUT_INVALID && commands == before);
    FEED("LED OFF\x7f\n"); CHECK(last == WATCH_LED_INPUT_INVALID && commands == before);
    FEED("LED ON\b\n"); CHECK(last == WATCH_LED_INPUT_INVALID && commands == before);
    char boundary[64]; memset(boundary, ' ', 63); memcpy(boundary, "HELP", 4); boundary[63] = '\n';
    feed(boundary, sizeof boundary); CHECK(commands == before + 1 && command.kind == WATCH_LED_CMD_HELP);
    char oversized[100]; memset(oversized, 'x', sizeof oversized); feed(oversized, sizeof oversized);
    FEED("LED ON\r\n"); CHECK(last == WATCH_LED_INPUT_TOO_LONG && commands == before + 1);
    FEED("LED OFF\n"); CHECK(commands == before + 2 && command.kind == WATCH_LED_CMD_OFF);
    FEED("SET INTERVAL "); watch_led_commands_reset(&parser, true);
    FEED("LED ON\r\n"); CHECK(commands == before + 2);
    FEED("LED ON\n"); CHECK(commands == before + 3 && command.kind == WATCH_LED_CMD_ON);
    /* Every possible transport split gives exactly one identical command. */
    const char text[] = "SET INTERVAL 750\r\n";
    for(size_t split = 0; split <= sizeof(text) - 1U; ++split) {
        watch_led_commands_reset(&parser, false); before = commands;
        feed(text, split); feed(text + split, sizeof(text) - 1U - split);
        CHECK(commands == before + 1 && command.kind == WATCH_LED_CMD_INTERVAL && command.interval_ms == 750);
    }
    FEED("REQ 7 HELLO\n"); CHECK(command.kind == WATCH_LED_CMD_DEVICE_REQUEST && command.sequence == 7 && !strcmp(command.device_command, "HELLO"));
    FEED("REQ 4294967295 STATUS\n"); CHECK(command.sequence == UINT32_MAX && !strcmp(command.device_command, "STATUS"));
    FEED("REQ x HELLO\n"); CHECK(last == WATCH_LED_INPUT_INVALID);
    FEED("REQ 4294967296 HELLO\n"); CHECK(last == WATCH_LED_INPUT_INVALID);
    FEED("REQ -1 HELLO\n"); CHECK(last == WATCH_LED_INPUT_INVALID);
    FEED("REQ 7 HELLO extra\n"); CHECK(last == WATCH_LED_INPUT_INVALID);
    FEED("REQ 0 BOOT\n"); CHECK(command.kind == WATCH_LED_CMD_DEVICE_REQUEST && command.sequence == 0 && !strcmp(command.device_command, "BOOT"));
    printf("PASS: %u transport-independent command parser checks\n", checks);
    return 0;
}
