#ifndef PARTIE_H
#define PARTIE_H

#include "joueur.h"

typedef struct partie {
    joueur * joueurs[4]; // les équipes sont faites en fonction de la parité
    int port_multi; // port de multidiffusion
    char * addr_multi; // adresse de multidiffusion FF12:
    int port; // port pour recevoir des messages
    int equipes; // vaut 1 pour une partie en équipe et 0 sinon
} partie;


typedef struct arg_serve{
    partie * partie4v4; // les parties qui ont encore de la place -> ajouter partie2v2
    partie * partie2v2;
    int sock;
} arg_serve;


joueur * ajoute_joueur(partie *, int);

partie * nouvelle_partie(int);

int partie_prete(partie);

int partie_remplie(partie);


void *serve_partie(void *);

#endif