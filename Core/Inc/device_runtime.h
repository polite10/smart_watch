#ifndef DEVICE_RUNTIME_H
#define DEVICE_RUNTIME_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
typedef const char *(*device_command_handler_t)(const char *command);
void device_runtime_init(const char *app, device_command_handler_t handler);
int device_runtime_format_reply(uint32_t sequence, const char *command,
                               char *message, size_t capacity, bool *request_boot);
void device_runtime_boot_reply_complete(void);
void device_runtime_poll(void);
#endif
