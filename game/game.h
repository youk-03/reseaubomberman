#ifndef GAME_H
#define GAME_H

#include "myncurses.h"
#include <stdint.h>
#include "../format_messages.h"
#include "../client/auxiliaire.h"

void maj_grid (full_grid_msg *msg, board *b);
full_grid_msg* full_grid_req (board *b, unsigned int num);
modified_cases_msg* modified_grid_req(unsigned int num, uint8_t nb);
message_partie_client* perform_action_req(board* b, pos* p, ACTION a,bomblist *list, info_joueur *info_joueur, unsigned int num);
full_grid_msg* from_clientreq_tofullgridreq(board *board, message_partie_client *msg, unsigned int num, pos* pos );


#endif