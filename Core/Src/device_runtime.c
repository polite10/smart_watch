/* Device Studio reply formatting shares the LED transport's RX and TX DMA. */
#include "stm32u5xx_hal.h"
#include "device_runtime.h"
#include "device_contract.h"
#include <stdio.h>
#include <string.h>

static const char *app_name;
static device_command_handler_t app_handler;
static volatile bool boot_armed;
static volatile uint32_t boot_reply_at;

void device_runtime_init(const char *app, device_command_handler_t handler)
{
    app_name = app;
    app_handler = handler;
    boot_armed = false;
}

int device_runtime_format_reply(uint32_t sequence, const char *command,
                               char *message, size_t capacity, bool *request_boot)
{
    const char *state = "{}", *error = "";
    *request_boot = false;
    if(!strcmp(command, "HELLO") || !strcmp(command, "STATUS")) {
        if(app_handler) state = app_handler("STATUS");
    } else if(!strcmp(command, "BOOT")) {
        if(!device_boot_installed()) error = "bootloader_missing";
        else *request_boot = true;
    } else if(app_handler) state = app_handler(command);
    else error = "unsupported_command";
    if(!state) state = "{}";
    return snprintf(message, capacity,
        "{\"seq\":%lu,\"app\":\"%s\",\"version\":1,\"protocol\":1,\"boot\":%d,\"ready\":true,\"error\":\"%s\",\"state\":%s}\r\n",
        (unsigned long)sequence, app_name ? app_name : "Smartwatch", device_boot_installed() ? 1 : 0, error, state);
}

/* Called only after the matching BOOT reply's final UART stop bit was sent. */
void device_runtime_boot_reply_complete(void)
{
    boot_reply_at = HAL_GetTick();
    boot_armed = true;
}

void device_runtime_poll(void)
{
    if(boot_armed && HAL_GetTick() - boot_reply_at >= 20U) {
        boot_armed = false;
        device_request_boot();
    }
}
