#include "viewmodels/watch_viewmodels.h"
#include <stdio.h>
#include <string.h>
static watch_calculator_state_t state;
const watch_calculator_state_t *watch_calculator_vm_state(void) { return &state; }
void watch_calculator_vm_init(void) { memset(&state,0,sizeof state); strcpy(state.text,"0"); }
void watch_calculator_vm_key(const char *key)
{
    if(!key) return;
    size_t len = strlen(state.expression);
    if(!strcmp(key, "C")) { state.expression[0] = '\0'; state.result = false; }
    else if(!strcmp(key, "DEL")) { if(len) state.expression[len - 1] = '\0'; state.result = false; }
    else if(!strcmp(key, "=")) {
        double value;
        if(watch_calculate(state.expression, &value)) {
            /* Plain decimal output can be used for the next calculation. */
            snprintf(state.expression, sizeof state.expression, "%.6f", value);
            char *dot = strchr(state.expression, '.');
            if(dot) {
                char *tail = state.expression + strlen(state.expression) - 1;
                while(tail > dot && *tail == '0') *tail-- = '\0';
                if(tail == dot) *tail = '\0';
            }
            state.result = true;
        } else {
            state.small = true;
            snprintf(state.text,sizeof state.text,"Gecersiz islem");
            state.expression[0] = '\0'; state.result = false;
            return;
        }
    } else {
        if(state.result && (key[0] == '.' || (key[0] >= '0' && key[0] <= '9'))) {
            state.expression[0] = '\0'; len = 0;
        }
        state.result = false;
        if(len + strlen(key) < sizeof state.expression) strcat(state.expression, key);
    }
    snprintf(state.text,sizeof state.text,"%s",state.expression[0] ? state.expression : "0");
    state.small = strlen(state.expression)>10;
}
