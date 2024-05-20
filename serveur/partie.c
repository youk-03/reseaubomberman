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

#define SIZE_MESS 100

pthread_mutex_t verrou_partie = PTHREAD_MUTEX_INITIALIZER; // utiliser pour ajouter des joueurs à une partie

static int port_nb = 24000;
static int addr_nb = 1;

int socks[100];

void ajoute_client(int c){
    for (int i=0; i<100; i++){
        if (socks[i]==-1){
            socks[i]=c;
            return;
        }
    }
    printf("Nombre maximum de sockets atteint");
}

void init_socks(){
    memset(socks, -1, 100*sizeof(int));
}

void termine (int sig){
    printf("interruption du programme\n");
    for (int i=0; i<100; i++){
        if(socks[i]>=0) {
            close(socks[i]);
            printf("sock %d fermée\n", socks[i]);
        }
    }
    exit(0);
}


joueur * ajoute_joueur(partie ** pp, int sock){ // Peut-être bouger dans un autre fichier
    partie * p = * pp;
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

    pthread_mutex_lock(&verrou_partie);  // TODO : vérifier que le pointeur se mette bien à jour
    p = nouvelle_partie(p->equipes);
    * pp = p;
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
    // TODO : faire une fonction qui ferme les sockets tcp des joueurs et qui free correctement


    partie p = *(partie *)arg;
    board *board_diff = malloc(sizeof(board));
    setup_board(board_diff);
    board *board = p.board;
    unsigned int num_msg = 0;
    unsigned int tick = 10*1000;
    unsigned int cpt_freq = 0;
    unsigned int cpt_sec = 0;
    unsigned int freq = 30*1000;

    bool death[4];
    death[0] = false;
    death[1] = false;
    death[2] = false;
    death[3] = false;

    bool send[4];
    send[0] = false;
    send[1] = false;
    send[2] = false;
    send[3] = false;

    bomblist *list =create_list(10);

    while(!partie_prete(p)){
    }

    // ici, thread pour le tchat

    pthread_t thread_tchat;
    if (pthread_create(&thread_tchat, NULL, serve_tchat, arg)) {
	    perror("pthread_create");
	    free(arg);
        return NULL;
    }  

    // socket multidiffusion
    
    int  sock_multi = socket(PF_INET6, SOCK_DGRAM, 0);
    ajoute_client(sock_multi);
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
    set_grid(board,2,board->h-1,CHARACTER3); //joueur3
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
    // socket udp (pour recevoir les messages des clients)

    // int sock_udp = socket(PF_INET6, SOCK_DGRAM, 0);
    // ajoute_client(sock_udp);
    // struct sockaddr_in6 adrudp;
    // memset(&adrudp, 0, sizeof(gradr));
    // adrudp.sin6_family = AF_INET6;
    // adrudp.sin6_addr = in6addr_any;
    // adrudp.sin6_port = htons(p.port);
    // if (bind(sock_udp, (struct sockaddr *)&adrudp, sizeof(adrudp))<0){
    //     perror("bind");
    //     free(arg);
    //     close(sock_multi);
    //     return NULL;
    // }



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
    ajoute_client(sock_udp_recv);
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

             modified_cases_msg = maj_board(list_of_msg,list,&p,num_msg, death); //board modified

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
        if(maj_bomb(list,tick,board, death)){
            for(int i=0; i<4; i++){
                if(death[i]){
                    //envoie message de mort
                    memset(buffer,0,sizeof(full_grid_msg));
                    req = full_grid_dead(17+i);
                    memcpy(buffer,req,sizeof(full_grid_msg));
                    s = sendto(sock_multi, buffer,sizeof(full_grid_msg), 0, (struct sockaddr*)&gradr, sizeof(gradr));
                    if (s < 0) perror("erreur send !!!");

                    memset(buffer, 0, sizeof(full_grid_msg));
                    free(req); 
                }
            }
            switch(is_finished(&p, death)){
                case -1: break; //pas fini
                case 0:printf("0 a gagné\n"); break;
                case 1:printf("1 a gagné\n"); break;
                case 2:printf("2 a gagné\n"); break;
                case 3:printf("3 a gagné\n"); break;
                case 4:printf("team 1-3 a gagné\n"); break;
                case 5:printf("team 0-2 a gagné\n"); break;
            }
            if(is_finished(&p, death)>=0) {
                printf(" partie finie\n");
                break;
            }
        }  //rempli le tableau de bool death[4] pour capter si des joueurs mort ou non
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


    //free(req);
    //req= NULL;
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

    // à faire : fin de partie -> tout free et fermer correctement + vérifier fin du thread tchat

    return NULL;
}

void fin_partie(int sock, u_int16_t gagnant, partie p){ // gagnant correspond soit à l'id du joueur ou de l'équipe gagnante
    uint16_t * message = malloc(sizeof(uint16_t));
    if (p.equipes == 0){ // fin partie dans équipes
        * message = htons(gagnant<<13|15);
    } else { // fin partie en équipes
        * message = htons(gagnant<<15|16);
    }

    int size = sizeof(* message);
    int sent = 0 ;
    while(sent<size) {
        int s=send(sock,message+sent,size-sent,0) ;  // à vérifier que ça marche :)
        if (s == -1) {
            perror("erreur envoi fin de partie") ;
            free(message);
            return ;
        } 
        sent+=s;
    } 

    free(message);

}

void *serve_tchat(void * arg) {
    printf("Début du tchat \n");
    partie p = *(partie *)arg;

    struct pollfd *pfds = malloc(sizeof(*pfds) * 4); // free
    memset(pfds, 0, sizeof(*pfds) * 4);

    for (int i=0; i<4; i++) {
        pfds[i].fd = p.joueurs[i]->sock;
        pfds[i].events = POLLIN;
    }

    while(1) {
        int poll_cpt = poll(pfds, 4, 0);

        if (poll_cpt == -1) {
            perror("erreur poll");
            return NULL;
        }

        for (int i=0; i<4; i++){
            if (pfds[i].revents & POLLIN) {
                printf("message %d ;\n",0);
                char buf[SIZE_MESS];
                memset(buf, 0, SIZE_MESS);

                // On reçoit
                message_tchat * mess = malloc(sizeof(message_tchat));
                memset(mess, 0, sizeof(message_tchat));

                // On reçoit les premiers champs d'abords
                int recu = 0;
                while(recu<3) { 
                    int r = recv(pfds[i].fd, buf+recu, 3-recu, 0);
                    if (r<0){
                        perror("erreur lecture tchat entête");
                        return NULL;
                    }
                    if (r==0) {
                        printf("serveur off");
                        return NULL;
                    }
                    recu += r;
                }

                memcpy(mess,(message_tchat*)&buf,sizeof(message_tchat));
                // vérifier le premier champs
                // lire la taille du message
                uint16_t codereq_id_eq = ntohs(mess->CODEREQ_ID_EQ);
                uint16_t codereq = codereq_id_eq & 0b1111111111111; // pour lire 13 bits
                // TODO : égal à 7 ou 8
                // id
                uint16_t id = ( codereq_id_eq >>13) & 0b11;
                if (id > 3){
                    printf("message tchat, erreur valeur id dans message reçu\n");
                    return NULL;
                }
                // eq
                uint16_t eq;
                if (codereq==8){
                    eq = ( codereq_id_eq >> 13) & 0b1;
                    if (eq != id%2) {
                        printf("message tchat, erreur valeur eq dans message reçut\n");
                        return NULL;
                    }
                }

                uint8_t len = mess->LEN;
                char buf_data[len+1];
                memset(buf_data, 0, len+1);

                printf("message tchat, codereq: %d, size : %d \n",codereq, len);

                // on reçoit la data
                recu = 0;
                while(recu<len) { 
                    int r = recv(pfds[i].fd, buf_data+recu, len-recu, 0);
                    printf("%s\n",buf_data);
                    if (r<0){
                        perror("erreur lecture tchat data");
                        return NULL;
                    }
                    if (r==0) {
                        printf("serveur off");
                        return NULL;
                    }
                    recu += r;
                }
                printf("message tchat : %s\n",buf_data);

                uint16_t dest;
                if (codereq == 7) dest = 13;
                else dest = 14;

                mess->CODEREQ_ID_EQ=htons((eq << 15) | (id << 13) | (dest));
                int size = 3 + len;
                char* serialized_msg = malloc(size); 
                memset(serialized_msg, 0, sizeof(size));
                memcpy(serialized_msg, mess,3); 
                memcpy(serialized_msg+3, buf_data,len );


               /* for (size_t i = 0; i < size; i++) {
                    printf("%02X ", serialized_msg[i]); //hex
                }*/

                // On envoie aux autres
                char bufsend[SIZE_MESS+5];
                memset(bufsend, 0, SIZE_MESS);
                sprintf(bufsend, "J%d : %s", i, buf);
                for (int j=0; j<4; j++){
                    if (j!=i && ((codereq!=8) || j%2==eq)) {
                        int ecrit = 0;  
                        while (ecrit<size){
                            ecrit += send(pfds[j].fd, serialized_msg + ecrit, size-ecrit, 0); 
                        }   
                    }
                }

                free(mess);
                free(serialized_msg);

            }
        }
    }
}