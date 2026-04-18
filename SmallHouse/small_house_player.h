#ifndef SMALL_HOUSE_PLAYER_H
#define SMALL_HOUSE_PLAYER_H

#include "small_house_state.h"

void small_house_player_init(SmallHouseState *state);
void small_house_play_selected_mjpeg(SmallHouseState *state, int mjpeg_index);

#endif
