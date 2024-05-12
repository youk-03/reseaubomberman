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
#include "../game/myncurses.h"
#include "../game/game.h"

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

            if (partie_remplie(*p)){
                printf("Lancement partie \n");
                pthread_t thread_partie;
                if(pthread_create(&thread_partie, NULL, serve_partie, p)){
                    perror("pthread_create : nouvelle partie");
                }
            }
            pthread_mutex_unlock(&verrou_partie);
            return j; //TODO : lancer la partie si elle est remplie ici plutôt
        }
    }
    pthread_mutex_unlock(&verrou_partie);

    pthread_mutex_lock(&verrou_partie);
    p = nouvelle_partie(p->equipes);
    joueur * j = nouveau_joueur(sock, 0);
    p->joueurs[0] = j;
    printf("Joueur ajouté à la partie \n");
    pthread_mutex_unlock(&verrou_partie);
    return j;
}

partie * nouvelle_partie(int equipes){ //ICI INITIALISER LE BOARD
    partie * p = malloc(sizeof(partie));
    memset(p, 0, sizeof(partie));
    p->addr_multi=malloc(sizeof(char)*60); //todo: penser à free
    if (p->addr_multi == NULL) {
        perror("erreur de malloc");
    }
    p->port = port_nb;
    port_nb ++; // -> à retirer si on veut tester la multidiffusion
    p->port_multi = port_nb;
    p->equipes = equipes;
    port_nb++;  // -> à retirer si on veut tester la multidiffusion
    char str[50];
    sprintf(str, "FF12:ABCD:1234:%d:AAAA:BBBB:CCCC:DDDD",addr_nb++ ); // -> remplacer par la ligne d'en dessous pour tester multidiffusion
    //sprintf(str, "FF12:ABCD:1234:%d:AAAA:BBBB:CCCC:DDDD",addr_nb);
    //p->addr_multi = str;
    memcpy(p->addr_multi,str,sizeof(str));

    p->board = malloc(sizeof(board)); //PAS ENCORE FREE POUR LE MOMENT Y PENSER
    setup_board(p->board); //board initial
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

int partie_remplie(partie p){
    for (int i=0; i<4; i++){

        if (p.joueurs[i]==NULL) {
            return 0;
        }
    }
    return 1;
}

void *serve_partie(void * arg) { // fonction pour le thread de partie



    partie p = *(partie *)arg;
    board *board = p.board;

    while(!partie_prete(p)){
    } //attente active peut-être faire autrement 
    
    int  sock_multi = socket(AF_INET6, SOCK_DGRAM, 0);
    struct sockaddr_in6 gradr;
    memset(&gradr, 0, sizeof(gradr));
    gradr.sin6_family = AF_INET6;
    printf("adresse avant multidiffusion : %s\n", p.addr_multi);
    inet_pton(AF_INET6, p.addr_multi, &gradr.sin6_addr);
    gradr.sin6_port = htons(p.port_multi);

    int ifindex = if_nametoindex ("eth0");
    gradr.sin6_scope_id = ifindex;


    // multidiffuse la grille initiale

    full_grid_msg *req =full_grid_req(board, 5); //--> FREE REQ
    char *buffer = malloc(sizeof(full_grid_msg)+1); //FREE MESSAGE
    memcpy(buffer,req,sizeof(full_grid_msg));

    int s = sendto(sock_multi, buffer, sizeof(full_grid_msg), 0, (struct sockaddr*)&gradr, sizeof(gradr));
    if (s < 0)
        perror("erreur send !!!");


    free(req);
    req= NULL;
    free(buffer);
    buffer = NULL;


    // déroulement de la partie

    // TODO : recevoir action et agir
    // + multidiffusion de la grille régulièremement (un autre thread ?)


    return NULL;
}