#ifndef WATCH_RECORDS_SERVICE_H
#define WATCH_RECORDS_SERVICE_H
#include "models/watch_records.h"
#include "models/watch_model.h"
/* Platform ports: share the existing atomic journal without changing user data. */
bool watch_storage_records_load(watch_records_t *records);
bool watch_storage_records_save(const watch_data_t *data, const watch_records_t *records);
void watch_records_service_init(void);
void watch_records_service_update(watch_record_game_t game, uint32_t result);
bool watch_records_service_save(void);
#endif
