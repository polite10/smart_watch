#include "viewmodels/watch_viewmodels.h"
#include <stdio.h>
static watch_alarm_editor_t editor;
watch_alarm_row_t watch_alarm_vm_row(unsigned index)
{
    watch_alarm_row_t row={0}; if(index>=WATCH_ALARMS) return row;
    const watch_alarm_t *a=&watch_model_data()->alarms[index];
    row.hour=a->hour; row.minute=a->minute; row.daily=a->daily; row.enabled=a->enabled;
    snprintf(row.time,sizeof row.time,"%02u:%02u",a->hour,a->minute);
    snprintf(row.repeat,sizeof row.repeat,"%u / %s",index+1,a->daily ? "Her gun" : "Bir kez");
    snprintf(row.toggle,sizeof row.toggle,"%s",a->enabled ? "Acik" : "Kapali"); return row;
}
const watch_alarm_editor_t *watch_alarm_vm_editor(void) { return &editor; }
void watch_alarm_vm_open(unsigned index)
{
    if(index>=WATCH_ALARMS) return;
    const watch_alarm_t *a=&watch_model_data()->alarms[index];
    editor.index=index; editor.hour=a->hour; editor.minute=a->minute; editor.daily=a->daily;
    snprintf(editor.repeat,sizeof editor.repeat,"Tekrar: %s",editor.daily ? "Her gun" : "Bir kez");
}
void watch_alarm_vm_repeat(void)
{
    editor.daily=!editor.daily;
    snprintf(editor.repeat,sizeof editor.repeat,"Tekrar: %s",editor.daily ? "Her gun" : "Bir kez");
}
bool watch_alarm_vm_toggle(unsigned index)
{ return watch_model_alarm_toggle(index) && watch_app_vm_save(); }
bool watch_alarm_vm_save(uint8_t hour, uint8_t minute)
{
    editor.hour=hour; editor.minute=minute;
    return watch_model_alarm_set(editor.index,hour,minute,editor.daily) && watch_app_vm_save();
}
