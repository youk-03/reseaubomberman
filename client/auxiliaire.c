
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "../format_messages.h"


typedef struct info_joueur {
    int mode ;
    int dest ;
    int id ;
    int team ; 
} info_joueur ;

int join_req(message_debut_client* msg_client, int mode) { //1 si solo, 2 si équipes,à bouger vers un autre fichier
  msg_client->CODEREQ_ID_EQ = htons(mode);
  return 0;
}

int ready_req(message_debut_client* msg_client, info_joueur * info_joueur) {
  msg_client->CODEREQ_ID_EQ = htons((info_joueur->team << 15) | (info_joueur->id << 13) | (info_joueur->mode));
  return 0;
}

int move_req(message_partie_client * msg_client, info_joueur * info_joueur) { //todo: voir comment on passe num et action
    int mode; 
    if (info_joueur->mode == 1) {
        mode=5;
    } else {
        mode=6;
    }

    int eq=0;
    if (mode == 6) {
        eq=info_joueur->team;
    }

    msg_client->CODEREQ_ID_EQ=htons((eq << 15) | (info_joueur->id << 13) | mode );
    //à compléter 
    return 0;
}

// int chat_req(message_tchat_client* msg_client, info_joueur * info_joueur, char * msg) {
//     memset(msg_client,0,sizeof(message_tchat_client));
//     int len = strlen(msg);
//     msg_client->CODEREQ_ID_EQ = htons((info_joueur->team) <<15 | (info_joueur->id) << 13 | (info_joueur->dest));
//     msg_client->LEN_DATA[0] =( len << 8 | msg[0] ) ;
//     //todo: voir comment remplir le reste
//     return 0 ;
// }

int string_to_struct(message_debut_serveur * serv_msg, char * serialized_serv_msg) { 
  memset(serv_msg, 0, sizeof(message_debut_serveur));
  memcpy(serv_msg,serialized_serv_msg,sizeof(message_debut_serveur));
  return 0 ; 
}

int struct_to_string(message_debut_client * msg_cli, char * buf, int mode) {
  memset(msg_cli, 0, sizeof(message_debut_client));
  join_req(msg_cli,mode);
  memcpy(buf,msg_cli,sizeof(&msg_cli)); 
  return 0;
}

int set_addrdiff(struct in6_addr * adrmdiff_convert, message_debut_serveur * serv_msg) {
  memset(adrmdiff_convert,0,sizeof(struct in6_addr));
  memcpy(adrmdiff_convert,serv_msg->ADRMDIFF,8*sizeof(uint16_t));
  return 0 ;
  }

int info_check(info_joueur * info_joueur, message_debut_serveur * serv_msg ) {

  info_joueur->mode=ntohs(serv_msg->CODEREQ_ID_EQ) & 0b1111111111111; 

  if (info_joueur->mode==9) {
    info_joueur->mode=3;
   } else if (info_joueur->mode==10) {
    info_joueur->mode=4;
   } else {
     perror("erreur réception codereq"); 
     return 1 ;
   }

  info_joueur->id=ntohs(serv_msg->CODEREQ_ID_EQ)>>13 & 0b11;
  if (info_joueur->id<0 || info_joueur->id>3) {
    perror ("erreur réception id");
    return 1 ;
  }

  info_joueur->team= ntohs((serv_msg->CODEREQ_ID_EQ)) >> 13 & 0b1 ;
  return 0;
}