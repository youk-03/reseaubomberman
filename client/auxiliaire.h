#ifndef AUXILIAIRE_H
#define AUXILIAIRE_H

#include "../format_messages.h"

typedef struct info_joueur {
    int mode ;
    int id ;
    int team ; 
} info_joueur ;


int join_req(message_debut_client* msg_client, int mode) ;

int ready_req(message_debut_client* msg_client, info_joueur * info_joueur);

int string_to_struct(message_debut_serveur * serv_msg, char * serialized_serv_msg);

int struct_to_string(message_debut_client * msg_cli, char * buf, int mode);

int set_addrdiff(struct in6_addr * adrmdiff_convert, message_debut_serveur * serv_msg);

int info_check(info_joueur * info_joueur, message_debut_serveur * serv_msg );



#endif