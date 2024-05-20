#ifndef JOUEUR_H
#define JOUEUR_H

#define BUF_SIZE 256

int * socks;

typedef struct joueur{
    int sock; // TCP
    int id; // entre 0 et 3
    int pret;
} joueur;

void ajoute_client(int c);

joueur * nouveau_joueur(int, int);

void *serve(void *);

#endif