#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "../format_messages.h"
#include "partie.h"

pthread_mutex_t verrou = PTHREAD_MUTEX_INITIALIZER; // utiliser pour ajouter des joueurs à une partie

static int port_nb = 24000;
static int addr_nb = 1;

joueur * ajoute_joueur(partie * p, int sock){ // Peut-être bouger dans un autre fichier
    pthread_mutex_lock(&verrou);
    for (int i=0; i<4; i++){
        if (p->joueurs[i]==NULL){
            joueur * j = nouveau_joueur(sock, i);
            p->joueurs[i] = j;
            printf("Joueur ajouté à la partie \n");
            pthread_mutex_unlock(&verrou);
            return j;
        }
    }
    pthread_mutex_unlock(&verrou);
    return NULL;
}

partie * nouvelle_partie(int equipes){
    partie * p = malloc(sizeof(partie));
    memset(p, 0, sizeof(partie));
    p->port = port_nb;
    port_nb ++;
    p->port_multi = port_nb;
    p->equipes = equipes;
    port_nb++;
    char str[50];
    sprintf(str, "FF12:ABCD:1234:%d:AAAA:BBBB:CCCC:DDDD",addr_nb++ );
    p->addr_multi = str;
    return p;
}

int partie_prete(partie p){
    for (int i=0; i<4; i++){
        if (p.joueurs[i]==NULL) return 0;
        if (!p.joueurs[i]->pret) return 0;
    }
    return 1;
}

void *serve_partie(void *p) {
    // fonction pour le thread de partie

    return NULL;
}