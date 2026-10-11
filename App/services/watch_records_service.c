#include "services/watch_records_service.h"
static bool dirty;
void watch_records_service_init(void)
{
    watch_records_t saved;
    watch_records_reset(watch_storage_records_load(&saved) ? &saved : 0);
    dirty=false;
}
void watch_records_service_update(watch_record_game_t game, uint32_t result)
{
    if(watch_records_update(game,result)) dirty=true;
}
bool watch_records_service_save(void)
{
    if(!dirty) return true;
    if(!watch_storage_records_save(watch_model_data(),watch_records_state())) return false;
    dirty=false;
    return true;
}
