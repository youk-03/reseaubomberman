#include "myncurses.h"
#include <stdint.h>
#include "../format_messages.h"
#ifndef GAME_H
#define GAME_H

void maj_grid (full_grid_msg *msg, board *b);
full_grid_msg* full_grid_req (board *b, unsigned int num);
modified_cases_msg* modified_grid_req(unsigned int num, uint8_t nb);


#endif