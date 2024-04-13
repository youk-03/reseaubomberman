#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/***  Messages du client ***/


/* intégrer et démarrer une partie */



/*  déroulement de la partie */


/* le tchat */



/*** Messages du serveur ***/

/* intégrer et démarer une partie */

typedef struct message_debut_serveur {
    uint16_t CODEREQ :13;
    uint16_t ID : 2;
    uint16_t EQ : 1;
    uint16_t PORTUDP;
    uint16_t PORTMDIFF;
    uint16_t ADRMDIFF[8];
} message_debut_serveur;

/* déroulement d'une partie */

/* le tchat */

/* la fin de partie */