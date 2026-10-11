#include "viewmodels/watch_games_vm.h"
#include "services/watch_records_service.h"
static bool records_saved=true;
const watch_records_t *watch_games_vm_records(void) { return watch_records_state(); }
bool watch_games_vm_save_records(void) { records_saved=watch_records_service_save();return records_saved; }
bool watch_games_vm_records_saved(void) { return records_saved; }
