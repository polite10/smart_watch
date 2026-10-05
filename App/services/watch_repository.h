#ifndef WATCH_REPOSITORY_H
#define WATCH_REPOSITORY_H
#include "models/watch_model.h"
/* Journal port implemented by the platform, replaceable in software tests. */
bool watch_storage_load(watch_data_t *data);
bool watch_storage_save(const watch_data_t *data);
void watch_model_init(void);
bool watch_model_save(void);
#endif
