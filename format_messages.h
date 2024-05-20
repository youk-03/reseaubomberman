#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#ifndef FORMAT_MESSAGES__H
#define FORMAT_MESSAGES__H



/***  Messages du client ***/


/* intégrer et démarrer une partie */


typedef struct message_debut_client {
    uint16_t CODEREQ_ID_EQ;
} message_debut_client ;


/*  déroulement de la partie */

typedef struct message_partie_client {
    uint16_t CODEREQ_ID_EQ;
    uint16_t NUM_ACTION;
} message_partie_client;


/* le tchat */

typedef struct message_tchat_client {
    uint16_t CODEREQ_ID_EQ;
    uint16_t LEN_DATA [8]; // pas sûre du type  // Je pense pas que ce soit le bon format, mais on peut utiliser le même format pour le serveur et le client
} message_tchat_client;



/*** Messages du serveur ***/

/* intégrer et démarer une partie */

typedef struct message_debut_serveur {
    uint16_t CODEREQ_ID_EQ; // si le jeu est en équipe, eq = id%2
    uint16_t PORTUDP;
    uint16_t PORTMDIFF;
    uint16_t ADRMDIFF[8]; // sinon unigned char ADRMDIFF[16] avec inet_pton
} message_debut_serveur;

/* déroulement d'une partie */

    /* Grille complète*/
typedef struct full_grid_msg {
    uint16_t CODEREQ_ID_EQ; // big endian
    uint16_t NUM; // big endian
    uint16_t HAUTEUR_LARGEUR;
    uint8_t CASE[1275]; // not sure it's the case for every size of screen else it needs to be done in a different way or smaller GRID_SIZE
} full_grid_msg;

    /* Cases modifiées */
typedef struct modified_cases_msg{
    uint16_t CODEREQ_ID_EQ; // big endian
    uint16_t NUM; // big endian
    uint8_t NB;
    //uint CASES; allouer un truc plus grand pour les cases puis l'envoyer comme ca ? 
    //pour la reception tjr cast en struct et recuperer dans un tab de uint8_t de 
    // taille nb
} modified_cases_msg;

typedef struct CASES{

uint16_t xy;
uint8_t action;

}CASES;



/* le tchat */

typedef struct message_tchat{ 
    uint16_t CODEREQ_ID_EQ; // big endian
    uint8_t LEN;
   // char * DATA; // pas sure du type
} message_tchat;

/* la fin de partie */

typedef struct message_fin_serveur{
    uint16_t CODEREQ_ID_EQ; // big endian
} message_fin_serveur;

#endif