#ifndef GAME_H
#define GAME_H

#include "myncurses.h"
#include <stdint.h>
#include "../format_messages.h"
#include "../client/auxiliaire.h"
#include "../serveur/partie.h"

typedef struct message_partie_client_liste{
    struct message_partie_client** msg_liste;
    int capacity;
    int length;
} message_partie_client_liste;

typedef struct caseholder{
    int length;
    int capacity;
    CASES ** caseh;
}caseholder;


message_partie_client_liste* create_list_msg (int capacity);
void add_liste_msg (message_partie_client_liste *list, message_partie_client* data);
int remove_msg(message_partie_client_liste *list, message_partie_client *data);
void free_list(message_partie_client_liste *list);
void empty_list_msg(message_partie_client_liste *list);


void maj_grid (full_grid_msg *msg, board *b);
full_grid_msg* full_grid_req (board *b, unsigned int num);
modified_cases_msg* modified_grid_req(unsigned int num, uint8_t nb);
message_partie_client* perform_action_req(board* b, pos* p, ACTION a,bomblist *list, info_joueur *info_joueur, unsigned int num);
void from_clientreq_toboard(board *board, message_partie_client *msg, pos* pos, bomblist *bomblist);
modified_cases_msg* maj_board(message_partie_client_liste* list, bomblist *bomblist, partie *p, unsigned int num, bool **death);


caseholder* get_difference(board *old, board *new);
void free_caseholder(caseholder *list);
int remove_case(caseholder *list, CASES *data);
void add_liste_case (caseholder *list, CASES* data);
caseholder* create_list_caseholder (int capacity);
void maj_grid_cases (caseholder *c, board *b);
void copy_board(board *old, board* new);

#endif