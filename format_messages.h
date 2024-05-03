#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/***  Messages du client ***/


/* intégrer et démarrer une partie */


typedef struct message_debut_client {
    uint16_t CODEREQ_IQ_EQ;
} message_debut_client ;


/*  déroulement de la partie */


/* le tchat */



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
typedef struct message_grille_complete {
    uint16_t CODEREQ_ID_EQ; // big endian
    uint16_t NUM; // big endian
    uint8_t HAUTEUR;
    uint8_t LARGEUR;
    uint8_t CASE[10]; // Modifier avec la taille du plateau
} message_grille_complete;

    /* Cases modifiées */
typedef struct message_cases_modifiees{
    uint16_t CODEREQ_ID_EQ; // big endian
    uint16_t NUM; // big endian
    uint8_t NB;
    uint CASES; // les cases transmises, je suis pas sûre du type
} message_cases_modifiees;


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