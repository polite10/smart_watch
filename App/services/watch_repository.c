#include "services/watch_repository.h"
void watch_model_init(void)
{
    watch_data_t saved;
    watch_model_reset(watch_storage_load(&saved) ? &saved : 0);
}
bool watch_model_save(void) { return watch_storage_save(watch_model_data()); }
