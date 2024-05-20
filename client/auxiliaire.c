
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "../format_messages.h"
#include "../game/myncurses.h"
#include "auxiliaire.h"


int send_message(int sock,info_joueur * info_joueur, char * message, int dest) {

  //int size = 3;
  message_tchat * mess = malloc(sizeof(message_tchat));

  if (mess==NULL) {
    return 1 ;
  }
  memset(mess,0,sizeof(message_tchat));
  if (info_joueur->mode == 3 && dest == 8 ) {
    perror("pas de coéquipier, impossible d'envoyer le message");
  }


  mess->CODEREQ_ID_EQ=htons((info_joueur->team << 15) | (info_joueur->id << 13) | (dest));
  //printf("%d\n", mess->CODEREQ_ID_EQ);
  mess->LEN=(uint8_t)(strlen(message)); // + 1?
  int size = 3 + strlen(message);
  char buf [size];//malloc(sizeof(message_tchat))  ; //16 pour codereq_id_eq, 8 pour len
  memset(buf, 0, size);
  memcpy(buf,mess,3);


  memcpy(buf+3,message,strlen(message)*sizeof(char));
  //envoi de la première partie

  //printf("contenu buffer : %s, size %d \n",buf,size);

  int sent = 0 ;
  while(sent<size) {
    int s=send(sock,buf+sent,size-sent,0) ; 
    if (s == -1) {
      perror("erreur envoi") ;
      return 1 ;
    } 
    sent+=s;
  } 


  //printf("taille msg %ld\n",sizeof(buf));
  //printf("envoye : %d \n",sent);
      

  // envoi du message 



  //char data [1+mess->LEN];
  //memcpy(buf,message,strlen(message)*sizeof(char));

//   sent=0 ;

 // printf("contenu buffer : %s, size %d \n",buf,size);

  

  //printf("message envoyé\n");
  free(mess);
  return 0;
}

int join_req(message_debut_client* msg_client, int mode) { //1 si solo, 2 si équipes,à bouger vers un autre fichier
  msg_client->CODEREQ_ID_EQ = htons(mode);
  return 0;
}

int ready_req(message_debut_client* msg_client, info_joueur * info_joueur) {
  msg_client->CODEREQ_ID_EQ = htons((info_joueur->team << 15) | (info_joueur->id << 13) | (info_joueur->mode));
  return 0;
}

int move_req(message_partie_client * msg_client, info_joueur * info_joueur, ACTION a, unsigned int num) { //TESTER POUR VOIR SI LA REQ S'ENVOIE BIEN
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
    char action;

    switch (a){
    case UP: action=0; break;
    case DOWN: action=2; break;
    case LEFT: action=3; break;
    case RIGHT: action=1; break;
    case PBOMB:action=4; break;
    case QUIT: action=5; break; //je vois pas l'interet de annuler la derniere demande de déplacement donc quit 
    }

    msg_client->NUM_ACTION = htons((action << 13) | num); 

    return 0;
}


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