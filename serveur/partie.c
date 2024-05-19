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

#define SIZE_MESS 100

pthread_mutex_t verrou_partie = PTHREAD_MUTEX_INITIALIZER; // utiliser pour ajouter des joueurs à une partie

static int port_nb = 24000;
static int addr_nb = 1;

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

partie * nouvelle_partie(int equipes){
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
    struct sockaddr_in6 gradr;
    memset(&gradr, 0, sizeof(gradr));
    gradr.sin6_family = AF_INET6;
    printf("adresse avant multidiffusion : %s\n", p.addr_multi);
    inet_pton(AF_INET6, p.addr_multi, &gradr.sin6_addr);
    gradr.sin6_port = htons(p.port_multi);

    int ifindex = if_nametoindex ("eth0");
    gradr.sin6_scope_id = ifindex;

    // socket udp (pour recevoir les messages des clients)

    int sock_udp = socket(PF_INET6, SOCK_DGRAM, 0);
    struct sockaddr_in6 adrudp;
    memset(&adrudp, 0, sizeof(gradr));
    adrudp.sin6_family = AF_INET6;
    adrudp.sin6_addr = in6addr_any;
    adrudp.sin6_port = htons(p.port);
    if (bind(sock_udp, (struct sockaddr *)&adrudp, sizeof(adrudp))<0){
        perror("bind");
        free(arg);
        close(sock_multi);
        return NULL;
    }



    // multidiffuse la grille initiale
    // TODO : envoyer les bonnes valeurs
    char buf[100];
    sprintf(buf, "grille initiale");
    int s = sendto(sock_multi, buf, strlen(buf), 0, (struct sockaddr*)&gradr, sizeof(gradr));
    if (s < 0)
        perror("erreur send !!!");


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