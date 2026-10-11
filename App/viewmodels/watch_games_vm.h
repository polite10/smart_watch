#ifndef WATCH_GAMES_VM_H
#define WATCH_GAMES_VM_H
#include "models/watch_records.h"
const watch_records_t *watch_games_vm_records(void);
bool watch_games_vm_save_records(void);
bool watch_games_vm_records_saved(void);
#endif
