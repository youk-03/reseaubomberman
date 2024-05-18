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

    //ajout des joueurs sur la grille
    set_grid(board,1,1,CHARACTER); //joueur1
    set_grid(board,board->w-1,1,CHARACTER2); //joueur2 
    set_grid(board,1,board->h-1,CHARACTER3); //joueur3
    set_grid(board,board->w-1,board->h-1,CHARACTER4); //joueur4

    //position des joueurs initiales
    p.joueurs[0]->pos = malloc(sizeof(pos));
    p.joueurs[1]->pos = malloc(sizeof(pos));
    p.joueurs[2]->pos = malloc(sizeof(pos));
    p.joueurs[3]->pos = malloc(sizeof(pos)); //FAIRE UNE FONC FREE PARTIE QUI FREE TOUT
    p.joueurs[0]->pos->x = 1;
    p.joueurs[0]->pos->y = 1;
    p.joueurs[1]->pos->x = p.board->w-1;
    p.joueurs[1]->pos->y = 1;
    p.joueurs[2]->pos->x = 1;
    p.joueurs[2]->pos->y = p.board->h-1;
    p.joueurs[3]->pos->x = p.board->w-1;
    p.joueurs[3]->pos->y = p.board->h-1;


    // multidiffuse la grille initiale


    full_grid_msg *req =full_grid_req(board, 5); 
    char *buffer = malloc(sizeof(full_grid_msg)+1); 
    memcpy(buffer,req,sizeof(full_grid_msg));

    int s = sendto(sock_multi, buffer, sizeof(full_grid_msg), 0, (struct sockaddr*)&gradr, sizeof(gradr));
    if (s < 0)
        perror("erreur send !!!");



    //recevoir une req d'un client et la print
    //declarer la socket udp pour recevoir
    char buf_recv[sizeof(message_partie_client)+1];
    memset(&buf_recv, 0, sizeof(message_partie_client)+1);

    int sock_udp_recv = socket(AF_INET6, SOCK_DGRAM, 0);
    struct sockaddr_in6 serv_udp_addr;
    serv_udp_addr.sin6_addr = in6addr_any;
    serv_udp_addr.sin6_family = AF_INET6;
    serv_udp_addr.sin6_port = htons(p.port);
    printf("port %d\n", p.port);
    
    if(bind(sock_udp_recv, (struct sockaddr*) &serv_udp_addr, sizeof(serv_udp_addr))) {
      perror("echec de bind serveur udp ecoute");
      exit(-1);
    }

    printf("reception requete client partie...\n");
    int r = recvfrom(sock_udp_recv, buf_recv, sizeof(message_partie_client), 0,NULL, NULL);
    if(r<0){
        perror(" recvfrom serveur udp erreur");
    }
    printf("requete client partie recu\n");

        for(int i=0; i<sizeof(message_partie_client); i++){
        printf("%02x", buf_recv[i]);
    }    

    printf("\n");


    free(req);
    req= NULL;
    free(buffer);
    buffer = NULL;

    //associer a tous les joueurs leurs positions FAIT
    //+ faire en sorte que l'ecran ncurses s'affiche pour les joueurs et qu'ils jouent bien leurs joueurs 


    // déroulement de la partie

    // TODO : recevoir action et agir
    // + multidiffusion de la grille régulièremement (un autre thread ?)


    return NULL;
}