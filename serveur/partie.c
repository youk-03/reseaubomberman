#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <net/if.h>
#include "../format_messages.h"
#include "partie.h"

pthread_mutex_t verrou_partie = PTHREAD_MUTEX_INITIALIZER; // utiliser pour ajouter des joueurs à une partie

static int port_nb = 24000;
static int addr_nb = 1;

joueur * ajoute_joueur(partie * p, int sock){ // Peut-être bouger dans un autre fichier
    pthread_mutex_lock(&verrou_partie);
    for (int i=0; i<4; i++){
        if (p->joueurs[i]==NULL){
            joueur * j = nouveau_joueur(sock, i);
            p->joueurs[i] = j;
            printf("Joueur ajouté à la partie \n");
            pthread_mutex_unlock(&verrou_partie);
            return j;
        }
    }
    pthread_mutex_unlock(&verrou_partie);
    return NULL;
}

partie * nouvelle_partie(int equipes){
    partie * p = malloc(sizeof(partie));
    memset(p, 0, sizeof(partie));
    p->addr_multi=malloc(sizeof(char)*60); //todo: penser à free
    if (p->addr_multi == NULL) {
        perror("erreur de malloc");
    }
    p->port = port_nb;
    port_nb ++;
    p->port_multi = port_nb;
    p->equipes = equipes;
    port_nb++;
    char str[50];
    sprintf(str, "FF12:ABCD:1234:%d:AAAA:BBBB:CCCC:DDDD",addr_nb++ );
    //p->addr_multi = str;
    memcpy(p->addr_multi,str,sizeof(str));
    return p;
}

int partie_prete(partie p){
    for (int i=0; i<4; i++){
        if (p.joueurs[i]==NULL) {
            return 0;
        }
        if (!p.joueurs[i]->pret) {
            return 0;
        }
    }
    return 1;
}

void *serve_partie(void * arg) { // fonction pour le thread de partie
    partie p = *(partie *)arg;
    
    int  sock_multi = socket(AF_INET6, SOCK_DGRAM, 0);
    struct sockaddr_in6 gradr;
    memset(&gradr, 0, sizeof(gradr));
    gradr.sin6_family = AF_INET6;
    inet_pton(AF_INET6, p.addr_multi, &gradr.sin6_addr);
    gradr.sin6_port = htons(p.port_multi);

    int ifindex = if_nametoindex ("eth0");
    gradr.sin6_scope_id = ifindex;


    // multidiffuse la grille initiale
    // TODO : envoyer les bonnes valeurs
    char buf[100];
    sprintf(buf, "grille initiale");
    int s = sendto(sock_multi, buf, strlen(buf), 0, (struct sockaddr*)&gradr, sizeof(gradr));
    if (s < 0)
        perror("erreur send\n");


    // déroulement de la partie

    // TODO : recevoir action et agir
    // + multidiffusion de la grille régulièremement (un autre thread ?)


    return NULL;
}