#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "../format_messages.h"

#ifdef MAC
#ifdef SO_REUSEADDR
#undef SO_REUSEADDR
#endif
#define SO_REUSEADDR SO_REUSEPORT
#endif

#define SIZE_BUF 256

static int port_nb = 24000;
static int addr_nb = 1;

pthread_mutex_t verrou = PTHREAD_MUTEX_INITIALIZER; // utiliser pour ajouter des joueurs à une partie

// Pour compiler : gcc -DMAC serveur.c -o serveur
typedef struct joueur{
    int sock;
    int id; // entre 0 et 3
    int pret;
} joueur;

typedef struct partie4v4{
    joueur * joueurs[4];
    int port_multi; // port de multidiffusion
    char * addr_multi; // adresse de multidiffusion FF12:
    int port; // port pour recevoir des messages
} partie4v4;

typedef struct arg_serve{
    partie4v4 * partie; // les parties qui ont encore de la place -> ajouter partie2v2
    int sock;
} arg_serve;


joueur * nouveau_joueur(int sock, int i){
    joueur * res = malloc(sizeof(joueur));
    res->sock = sock;
    res->id = i;
    res->pret = 0;
    return res;
}


joueur * ajoute_joueur(partie4v4* p, int sock){ // Peut-être bouger dans un autre fichier
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

partie4v4 * nouvelle_partie4v4(){
    partie4v4 * p = malloc(sizeof(partie4v4));
    memset(p, 0, sizeof(partie4v4));
    p->port = port_nb;
    port_nb ++;
    p->port_multi = port_nb;
    port_nb++;
    char str[50];
    sprintf(str, "FF12:ABCD:1234:%d:AAAA:BBBB:CCCC:DDDD",addr_nb++ );
    p->addr_multi = str;
    return p;
}

int partie_prete(partie4v4 p){
    for (int i=0; i<4; i++){
        if (p.joueurs[i]==NULL) return 0;
        if (!p.joueurs[i]->pret) return 0;
    }
    return 1;
}

void *serve(void *arg) { // mettre des limites d'attente sur les recv
    arg_serve a = *((arg_serve *)arg);
    int sock = a.sock;
  
  // ** recevoir premier message : type de partie **

    char buf[SIZE_BUF]; // TODO : à changer -> utiliser une struct
    memset(buf, 0, sizeof(buf));
    int recu = 0;
    while(recu<16) { // Je crois que c'est la taille du message
        int r = recv(sock, buf+recu, SIZE_BUF, 0);
        if (r<0){
            perror("recv");
            close(sock);
            free(arg);
            int *ret = malloc(sizeof(int));
            *ret = 1;
            pthread_exit(ret);
        }
        recu += r;
    }
    printf("recu : %s\n", buf);

    // Lire les données reçu et ajouter le joueur à une partie
    /* TODO : if ... partie4v4 else partie2v2 */
    
    joueur * j = ajoute_joueur(a.partie, sock);
    if (j==NULL){
        printf("Erreur le joueur n'a pas pu être ajouté");
        close(sock);
        free(arg);
        return NULL;
    }

    /* Envoyer données au joueur 
    - identifiant
    - numéro d'équipe ?
    - adresse IPv6
    - port multi
    - port
    */

    message_debut_serveur mess;
    memset(&mess, 0, sizeof(mess));
    mess.CODEREQ = 9; // TODO: j'ai pas géré le codage en big-endian
    mess.ID = j->id;
    mess.PORTUDP = htons(a.partie->port);
    mess.PORTMDIFF = htons(a.partie->port_multi);
    inet_pton(AF_INET6, a.partie->addr_multi, &mess.ADRMDIFF ); // C'est OK ?


    int ecrit = 0;
    while (ecrit<sizeof(mess)){
        ecrit += send(sock, &mess + ecrit, sizeof(mess)-ecrit, 0); // ???
    }
    printf("envoye\n");

    // attendre le message "prêt" du joueur

    memset(buf, 0, SIZE_BUF);

    recu = 0;
    while(recu<16) { // Je crois que c'est la taille du message
        int r = recv(sock, buf+recu, SIZE_BUF, 0);
        if (r<0){
            perror("recv");
            close(sock);
            free(arg);
            int *ret = malloc(sizeof(int));
            *ret = 1;
            pthread_exit(ret);
        }
        recu += r;
    }
    printf("recu : %s\n", buf);

    j->pret = 1 ; // mettre un mutex ? normalement seul ce thread est sensé écrire dans ce joueur



    close(sock); // a enlever peut-être un jour
    free(arg);
    return NULL;
}



//////////////////////////////////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[]){
    // *** Recevoir les messages de nouveaux joueurs ***

    int sock = socket(PF_INET6, SOCK_STREAM, 0);
    if(sock < 0){
        perror("creation socket début de partie");
        exit(1);
    }

    struct sockaddr_in6 address_sock;
    memset(&address_sock, 0, sizeof(address_sock));
    address_sock.sin6_family = AF_INET6;
    address_sock.sin6_port = htons(1024);
    address_sock.sin6_addr = in6addr_any;

    // polyvalence
    int optval = 0;
    int r = setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY, &optval, sizeof(optval));
    if (r < 0) 
        perror("erreur connexion IPv4 impossible");

    // Le numéro de port peut être utilisé en parallèle
    optval = 1;
    r = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    if (r < 0) 
        perror("erreur réutilisation de port impossible");

    r = bind(sock, (struct sockaddr *) &address_sock, sizeof(address_sock));
    if (r < 0) {
        perror("erreur bind");
        exit(2);
    }

    r = listen(sock, 0);
    if (r < 0) {
        perror("erreur listen");
        exit(2);
    }
    /* TODO : créer une partie vide puis et faire en sorte qu'on envoie 
    le pointeur vers les parties à compléter dans serve */
    partie4v4 * p4v4 = nouvelle_partie4v4();

    while(1){
        // accepte un client
        struct sockaddr_in6 addrclient;
        socklen_t size=sizeof(addrclient);
    
        int sock_client = accept(sock, (struct sockaddr *) &addrclient, &size);

        if (sock_client >= 0) {
            arg_serve * arg = malloc(sizeof(arg_serve));
            arg->partie = p4v4;
            arg->sock = sock_client;

            pthread_t thread;
            if (pthread_create(&thread, NULL, serve, arg)) {
	            perror("pthread_create");
	            continue;
            }  
      //*** affichage de l'adresse du client *** (à enlever ?)
            char nom_dst[INET6_ADDRSTRLEN];
            printf("client connecte : %s %d\n", inet_ntop(AF_INET6,&addrclient.sin6_addr,nom_dst,sizeof(nom_dst)), htons(addrclient.sin6_port));
        }

        printf("Partie prête : %d \n", partie_prete(* p4v4));
        if (partie_prete(* p4v4)){
            // lancer thread partie
            // mettre une nouvelle partie dans p4v4
        }

    }

    close(sock);
    return 0;



}