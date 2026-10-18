#include "services/watch_led_commands.h"
#include <string.h>

void watch_led_commands_reset(watch_led_commands_t *parser, bool discard)
{
    *parser = (watch_led_commands_t){.discard = discard};
}

static bool unsigned_number(const char *text, uint32_t *value)
{
    *value = 0;
    if(!*text) return false;
    for(const char *p = text; *p; ++p) {
        if(*p < '0' || *p > '9' || *value > (UINT32_MAX - (uint32_t)(*p - '0')) / 10U)
            return false;
        *value = *value * 10U + (uint32_t)(*p - '0');
    }
    return true;
}

static watch_led_input_result_t parse(char *line, watch_led_command_t *command)
{
    char *words[4];
    unsigned count = 0;
    while(*line) {
        while(*line == ' ' || *line == '\t') ++line;
        if(!*line) break;
        if(count == 4U) return WATCH_LED_INPUT_INVALID;
        words[count++] = line;
        while(*line && *line != ' ' && *line != '\t') ++line;
        if(*line) *line++ = '\0';
    }
    if(!count) return WATCH_LED_INPUT_PENDING;
    if(count == 3U && !strcmp(words[0], "REQ")) {
        if(!unsigned_number(words[1], &command->sequence) || strlen(words[2]) >= sizeof command->device_command)
            return WATCH_LED_INPUT_INVALID;
        command->kind = WATCH_LED_CMD_DEVICE_REQUEST;
        strcpy(command->device_command, words[2]);
    } else if(count == 1U && !strcmp(words[0], "HELP")) command->kind = WATCH_LED_CMD_HELP;
    else if(count == 2U && !strcmp(words[0], "LED")) {
        if(!strcmp(words[1], "ON")) command->kind = WATCH_LED_CMD_ON;
        else if(!strcmp(words[1], "OFF")) command->kind = WATCH_LED_CMD_OFF;
        else if(!strcmp(words[1], "BLINK")) command->kind = WATCH_LED_CMD_BLINK;
        else return WATCH_LED_INPUT_INVALID;
    } else if(count == 2U && !strcmp(words[0], "GET") && !strcmp(words[1], "STATUS"))
        command->kind = WATCH_LED_CMD_STATUS;
    else if(count == 2U && !strcmp(words[0], "RESET") && !strcmp(words[1], "PRESSES"))
        command->kind = WATCH_LED_CMD_RESET_PRESSES;
    else if(count == 3U && !strcmp(words[0], "SET") && !strcmp(words[1], "INTERVAL")) {
        uint32_t value = 0;
        bool overflow = false;
        for(const char *p = words[2]; *p; ++p) {
            if(*p < '0' || *p > '9') return WATCH_LED_INPUT_INVALID;
            if(value > (UINT32_MAX - (uint32_t)(*p - '0')) / 10U) overflow = true;
            if(!overflow) value = value * 10U + (uint32_t)(*p - '0');
        }
        if(overflow || value < WATCH_LED_INTERVAL_MIN_MS || value > WATCH_LED_INTERVAL_MAX_MS || value % 10U)
            return WATCH_LED_INPUT_RANGE;
        command->kind = WATCH_LED_CMD_INTERVAL;
        command->interval_ms = value;
    } else return WATCH_LED_INPUT_INVALID;
    return WATCH_LED_INPUT_COMMAND;
}

watch_led_input_result_t watch_led_commands_push(watch_led_commands_t *parser,
                                                uint8_t byte,
                                                watch_led_command_t *command)
{
    if(byte == '\r' || byte == '\n') {
        watch_led_input_result_t result = parser->error;
        if(!parser->discard && result == WATCH_LED_INPUT_PENDING) {
            parser->line[parser->length] = '\0';
            result = parse(parser->line, command);
        }
        watch_led_commands_reset(parser, false);
        return result;
    }
    if(parser->discard || parser->error != WATCH_LED_INPUT_PENDING) return WATCH_LED_INPUT_PENDING;
    if((byte < 32U && byte != '\t') || byte > 126U) parser->error = WATCH_LED_INPUT_INVALID;
    else if(parser->length == WATCH_LED_COMMAND_MAX) parser->error = WATCH_LED_INPUT_TOO_LONG;
    else parser->line[parser->length++] = (char)byte;
    return WATCH_LED_INPUT_PENDING;
}
