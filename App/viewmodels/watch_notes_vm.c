#include "viewmodels/watch_viewmodels.h"
#include <stdio.h>
#include <string.h>
static watch_notes_state_t state;
static const char *characters[]={"abcdefghijklmnopqrstuvwxyz","0123456789.,!?;:-_+*/=()@#","\n'"};
const watch_notes_state_t *watch_notes_vm_state(void) { return &state; }
void watch_notes_vm_open(unsigned index)
{
    if(index>=WATCH_NOTES) return;
    state=(watch_notes_state_t){.index=index};
    watch_notes_vm_change(watch_model_data()->notes[index]);
}
void watch_notes_vm_change(const char *text)
{
    if(!text) return;
    snprintf(state.draft,sizeof state.draft,"%s",text); state.confirm_delete=false;
    snprintf(state.hint,sizeof state.hint,"En fazla 191 karakter");
}
void watch_notes_vm_next_mode(void) { state.mode=(state.mode+1)%3; }
void watch_notes_vm_case(void) { state.uppercase=!state.uppercase; }
char watch_notes_vm_character(unsigned index)
{
    const char *keys=characters[state.mode];
    char c=index<strlen(keys) ? keys[index] : 0;
    if(state.uppercase && c>='a' && c<='z') c-='a'-'A';
    return c;
}
void watch_notes_vm_preview(unsigned index, char *text, unsigned size)
{
    if(index>=WATCH_NOTES) { if(size) text[0]=0; return; }
    char preview[23]; snprintf(preview,sizeof preview,"%.22s",watch_model_data()->notes[index]);
    for(char *p=preview;*p;++p) if(*p=='\n') *p=' ';
    snprintf(text,size,"%u. %s",index+1,preview[0] ? preview : "Yeni not");
}
bool watch_notes_vm_save(void)
{ return watch_model_note_set(state.index,state.draft) && watch_app_vm_save(); }
bool watch_notes_vm_delete(void)
{
    if(!state.confirm_delete) {
        state.confirm_delete=true; snprintf(state.hint,sizeof state.hint,"Silmek icin tekrar basin"); return false;
    }
    watch_model_note_set(state.index,""); return watch_app_vm_save();
}
