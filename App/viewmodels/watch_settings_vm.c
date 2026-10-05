#include "viewmodels/watch_viewmodels.h"
bool watch_settings_vm_twelve(void) { return watch_model_data()->twelve_hour; }
bool watch_settings_vm_light(void) { return watch_model_data()->light_theme; }
void watch_settings_vm_toggle_format(void)
{ watch_model_set_format(!watch_settings_vm_twelve()); watch_app_vm_save(); }
void watch_settings_vm_toggle_theme(void)
{ watch_model_set_theme(!watch_settings_vm_light()); watch_app_vm_save(); }
