#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <net/if.h>
#include <poll.h>
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
    p->addr_multi=malloc(sizeof(char)*60); //todo: penser à FREE
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

    p->board = malloc(sizeof(board)); 
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
    board *board_diff = malloc(sizeof(board));
    setup_board(board_diff);
    board *board = p.board;
    unsigned int num_msg = 0;
    unsigned int tick = 10*1000;
    unsigned int cpt_freq = 0;
    unsigned int cpt_sec = 0;
    unsigned int freq = 30*1000;

    bool death[4] = {false};

    //int tick = 30*1000;
    bomblist *list =create_list(10);

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

    //memcpy(board_diff, board, sizeof(board));
    copy_board(board_diff, board);


    // multidiffuse la grille initiale


    full_grid_msg *req =full_grid_req(board, 5);  
    char *buffer = malloc(sizeof(full_grid_msg)+1); 
    memcpy(buffer,req,sizeof(full_grid_msg));

    int s = sendto(sock_multi, buffer, sizeof(full_grid_msg), 0, (struct sockaddr*)&gradr, sizeof(gradr));
    if (s < 0)
        perror("erreur send !!!");

    free(req);
    req = NULL;
    memset(buffer, 0,sizeof(full_grid_msg)+1);



    //recevoir une req d'un client et la print
    //declarer la socket udp pour recevoir
    char buf_recv[sizeof(message_partie_client)+1];
    memset(&buf_recv, 0, sizeof(message_partie_client)+1);

    message_partie_client *msg_partie;

    message_partie_client_liste *list_of_msg = create_list_msg(10);//FREE AVEC EMPTY

    modified_cases_msg *modified_cases_msg; //free + free chaque tour de boucle
    caseholder *caseholder;

    int sock_udp_recv = socket(AF_INET6, SOCK_DGRAM, 0);
    struct sockaddr_in6 serv_udp_addr;
    serv_udp_addr.sin6_addr = in6addr_any;
    serv_udp_addr.sin6_family = AF_INET6;
    serv_udp_addr.sin6_port = htons(p.port);
    
    if(bind(sock_udp_recv, (struct sockaddr*) &serv_udp_addr, sizeof(serv_udp_addr))) {
      perror("echec de bind serveur udp ecoute");
      //tout FREE
      exit(-1);
    }

    //poll
    int fd_size = 2;
    struct pollfd *pfds = malloc(sizeof(*pfds) * fd_size);
    memset(pfds, 0, sizeof(*pfds) * fd_size);
    pfds[0].fd = sock_udp_recv;
    pfds[1].fd = sock_multi;
    pfds[0].events = POLLIN;
    pfds[1].events = POLLOUT;
    int poll_cpt = 0;

    while(1){
        poll_cpt = poll(pfds, fd_size, 0);

        if(poll_cpt == -1){
            perror("poll serveur");
            //go to error
            exit(-1);
        }

        if(poll_cpt == 0){
        dprintf(2,"r to do wtf\n");
        continue;
      }

      if(pfds[0].revents & POLLIN){

        //reception des requetes clients
        memset(buf_recv, 0, sizeof(message_partie_client));
        msg_partie= malloc(sizeof(message_partie_client));
        memset(msg_partie, 0, sizeof(message_partie_client));
                
        int r = recvfrom(sock_udp_recv, buf_recv, sizeof(message_partie_client), 0,NULL, NULL);
        if(r<0){
            perror(" recvfrom serveur udp erreur");
        }

        // for(int i=0; i<sizeof(message_partie_client); i++){
        //     printf("%02x", buf_recv[i]);
        // }    

        //  printf("\n");

        memcpy(msg_partie, buf_recv, sizeof(message_partie_client));
        add_liste_msg(list_of_msg, msg_partie);//ajoute a la liste le msg recu

      }

      if(pfds[1].revents & POLLOUT) {
        //envoie grille multidiffusion

        if(cpt_freq >= freq){
             cpt_freq = 0;

             modified_cases_msg = maj_board(list_of_msg,list,&p,num_msg); //board modified

    //         caseholder = get_difference(board_diff,board);
    //        //memcpy(board_diff, board, sizeof(board));
    //         copy_board(board_diff, board);

    //         if(caseholder){
    //                           printf("multidiff\n"); 

    //         char buf_caseholder[sizeof(caseholder)];
    //         memset(buf_caseholder,0, sizeof(caseholder));
    //         memcpy(buf_caseholder, caseholder, sizeof(caseholder));

    //         //declarer deux board 1 qui est modifié et l'autre non pour creer le differentiel des cases
    //         //Faire un fonc qui retourne ce differentiel et send ça

    //         char buf_modified_cases_msg[sizeof(struct modified_cases_msg)];
    //         memset(buf_modified_cases_msg, 0, sizeof(struct modified_cases_msg));
    //         memcpy(buf_modified_cases_msg, modified_cases_msg, sizeof(struct modified_cases_msg));

    //         for(int i=0; i<sizeof(caseholder); i++){
    //             printf("%02x", buf_caseholder[i]);
    //        }    
    // printf("bufcaseholder\n");

    //         //diffusion de modified grid

    //         //1er  send
    //         s = sendto(sock_multi, buf_modified_cases_msg,sizeof(struct modified_cases_msg), 0, (struct sockaddr*)&gradr, sizeof(gradr));
    //         if (s < 0)
    //             perror("erreur send !!!");

    //         num_msg++;
    //         num_msg = num_msg%8192; //2^13


    //         //2eme send 
    //         s = sendto(sock_multi, buf_caseholder,sizeof(caseholder), 0, (struct sockaddr*)&gradr, sizeof(gradr));
    //         if (s < 0)
    //             perror("erreur send !!!");
            
    //         free_caseholder(caseholder);

    //         }

             empty_list_msg(list_of_msg);
             free(modified_cases_msg);
        }
         //if(cpt_sec >= 1000000){ 
           if(cpt_sec >= freq){ 
            //printf("coucou\n");
            cpt_sec = 0;
            //diffusion full grid msg
            req = full_grid_req(board, num_msg);
            num_msg++;
            num_msg = num_msg%8192; //2^13

            memcpy(buffer,req,sizeof(full_grid_msg));
        //     for(int i=0; i<sizeof(full_grid_msg); i++){
        // printf("%02x", buffer[i]);
        // }    

        //printf("\n");

            s = sendto(sock_multi, buffer,sizeof(full_grid_msg), 0, (struct sockaddr*)&gradr, sizeof(gradr));
            if (s < 0)
                perror("erreur send !!!");

            memset(buffer, 0, sizeof(full_grid_msg));
            free(req); 
        }

        cpt_freq+=tick;
        cpt_sec+=tick;
        if(maj_bomb(list,tick,board, &death)) break; //rempli le tableau de bool death[4] pour capter si des joueurs mort ou non
        usleep(tick);
      }


    }

    // while(1){

    //     //serveur doit recevoir les requetes des clients et renvoyer toutes les freq sec les modified case
    //     //et toute les seconde des full_grid

    //     //garder un tick et faire un usleep tout les tick sec (correspond freq)
    //     //et garder un compteur pour savoir le nb de sec et quand 0 atteint multidiffuser
    //     //la grille

    // printf("reception requete client partie...\n");
    // int r = recvfrom(sock_udp_recv, buf_recv, sizeof(message_partie_client), 0,NULL, NULL);
    // if(r<0){
    //     perror(" recvfrom serveur udp erreur");
    // }
    // printf("requete client partie recu\n");

    //     for(int i=0; i<sizeof(message_partie_client); i++){
    //     printf("%02x", buf_recv[i]);
    // }    

    // printf("\n");

    // memcpy(msg_partie, buf_recv, sizeof(message_partie_client));

    // memset(buf_recv, 0, sizeof(message_partie_client));

    // //mettre a jour la pos du joueur d'id 


    // //envoie d'une nouvelle full grid msg apres modif de la premiere
    // int id = ntohs(msg_partie->CODEREQ_ID_EQ) >> 13 & 0b11;
    // printf("requete de : %d, ma pos x: %d, y:%d\n", id, p.joueurs[id]->pos->x,p.joueurs[id]->pos->y);

    // req =from_clientreq_tofullgridreq(board,msg_partie, num_msg, p.joueurs[id]->pos, list);  //maj le board avant
    // memcpy(buffer,req,sizeof(full_grid_msg));

    //         for(int i=0; i<sizeof(full_grid_msg); i++){
    //     printf("%02x", buffer[i]);
    // }    

    // printf("\n");

    // s = sendto(sock_multi, buffer, sizeof(full_grid_msg), 0, (struct sockaddr*)&gradr, sizeof(gradr));
    // if (s < 0)
    //     perror("erreur send !!!");

    // num_msg++;
    // num_msg = num_msg%8192; //2^13

    //     free(req);
    //     memset(msg_partie, 0, sizeof(message_partie_client));
    //     memset(buffer, 0, sizeof(full_grid_msg));
    // }


    free(req);
    req= NULL;
    free(buffer);
    buffer = NULL;
    free_board(board);
    empty_list(list);
    free(pfds);

    //associer a tous les joueurs leurs positions FAIT
    //+ faire en sorte que l'ecran ncurses s'affiche pour les joueurs et qu'ils jouent bien leurs joueurs 


    // déroulement de la partie

    // TODO : recevoir action et agir
    // + multidiffusion de la grille régulièremement (un autre thread ?)


    return NULL;
}