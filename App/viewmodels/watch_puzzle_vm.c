#include "viewmodels/watch_viewmodels.h"
#include <string.h>

static watch_puzzle_state_t puzzle_state;
static uint32_t generation;

const watch_puzzle_state_t *watch_puzzle_vm_state(void) { return &puzzle_state; }
void watch_puzzle_vm_new(uint32_t entropy)
{
    uint8_t previous[16]; memcpy(previous,puzzle_state.tiles,sizeof previous);
    do {
        watch_puzzle_new(&puzzle_state,entropy^(++generation*0x9E3779B9u));
    } while(!memcmp(previous,puzzle_state.tiles,sizeof previous));
}
bool watch_puzzle_vm_move(unsigned slot) { return watch_puzzle_move(&puzzle_state,slot); }
