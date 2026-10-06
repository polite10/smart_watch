#ifndef WATCH_IDA_VM_H
#define WATCH_IDA_VM_H
/* Static demonstration telemetry; no sensor or link claims. */
typedef struct { const char *time,*heading,*speed,*distance,*battery,*range,*link,*mode; } watch_ida_state_t;
const watch_ida_state_t *watch_ida_vm_state(void);
#endif
