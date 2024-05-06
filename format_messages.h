#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#ifndef FORMAT_MESSAGES__H
#define FORMAT_MESSAGES__H



/***  Messages du client ***/


/* intégrer et démarrer une partie */


typedef struct message_debut_client {
    uint16_t CODEREQ_IQ_EQ;
} message_debut_client ;


/*  déroulement de la partie */

typedef struct message_partie_client {
    uint16_t CODEREQ_ID_EQ;
    uint16_t NUM_ACTION;
} message_partie_client;


/* le tchat */

typedef struct message_tchat_client {
    uint16_t CODEREQ_ID_EQ;
    uint16_t LEN_DATA [8]; // pas sûre du type 
} message_tchat_client;



/*** Messages du serveur ***/

/* intégrer et démarer une partie */

typedef struct message_debut_serveur {
    uint16_t CODEREQ_ID_EQ;
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
    //uint CASES; // les cases transmises, je suis pas sûre du type
} modified_cases_msg;


/* le tchat */

typedef struct message_tchat_serveur{
    uint16_t CODEREQ_ID_EQ; // big endian
    uint8_t LEN;
    char * DATE; // pas sure du type
} message_tchat_serveur;

/* la fin de partie */

typedef struct message_fin_serveur{
    uint16_t CODEREQ_ID_EQ; // big endian
} message_fin_serveur;

#endif