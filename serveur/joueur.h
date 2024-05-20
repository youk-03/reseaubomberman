#ifndef JOUEUR_H
#define JOUEUR_H

#define BUF_SIZE 256
#include "../game/myncurses.h"

typedef struct joueur{
    int sock; // TCP
    int id; // entre 0 et 3
    int pret;
    pos *pos;
} joueur;

joueur * nouveau_joueur(int, int);

void *serve(void *);

#endif