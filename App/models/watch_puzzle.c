#include "models/watch_puzzle.h"

static uint32_t random_next(uint32_t *seed)
{
    *seed ^= *seed << 13;
    *seed ^= *seed >> 17;
    *seed ^= *seed << 5;
    return *seed;
}

static bool solved(const watch_puzzle_state_t *state)
{
    for(unsigned i=0; i<15; ++i) if(state->tiles[i] != i+1) return false;
    return state->tiles[15] == 0;
}

bool watch_puzzle_move(watch_puzzle_state_t *state, unsigned slot)
{
    if(slot >= WATCH_PUZZLE_CELLS || state->won || !state->tiles[slot]) return false;
    unsigned blank=state->blank;
    unsigned distance=(slot/4 > blank/4 ? slot/4-blank/4 : blank/4-slot/4)
                     +(slot%4 > blank%4 ? slot%4-blank%4 : blank%4-slot%4);
    if(distance != 1) return false;
    state->tiles[blank]=state->tiles[slot]; state->tiles[slot]=0;
    state->blank=slot;
    ++state->moves;
    state->won=solved(state);
    return true;
}

void watch_puzzle_new(watch_puzzle_state_t *state, uint32_t seed)
{
    if(!seed) seed=0xA341316Cu;
    *state=(watch_puzzle_state_t){0};
    for(unsigned i=0; i<16; ++i) state->tiles[i]=(i+1)%16;
    /* Fisher-Yates followed by parity repair: every generated board is solvable. */
    for(unsigned i=15; i>0; --i) {
        unsigned j=random_next(&seed)%(i+1);
        uint8_t tile=state->tiles[i]; state->tiles[i]=state->tiles[j]; state->tiles[j]=tile;
    }
    unsigned inversions=0, first=16, second=16;
    for(unsigned i=0; i<16; ++i) {
        if(!state->tiles[i]) { state->blank=i; continue; }
        if(first==16) first=i;
        else if(second==16) second=i;
        for(unsigned j=i+1; j<16; ++j)
            if(state->tiles[j] && state->tiles[i]>state->tiles[j]) ++inversions;
    }
    if((inversions+4-state->blank/4)%2==0) {
        uint8_t tile=state->tiles[first]; state->tiles[first]=state->tiles[second]; state->tiles[second]=tile;
    }
    /* Do not start with an already completed game. */
    if(solved(state)) { watch_puzzle_move(state,14); state->moves=0; }
}
