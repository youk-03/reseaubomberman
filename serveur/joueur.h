#ifndef JOUEUR_H
#define JOUEUR_H

#define BUF_SIZE 256

typedef struct joueur{
    int sock;
    int id; // entre 0 et 3
    int pret;
} joueur;

joueur * nouveau_joueur(int, int);

void *serve(void *);

#endif