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

#define BUF_SIZE 256

static int port_nb = 24000;
static int addr_nb = 1;

pthread_mutex_t verrou = PTHREAD_MUTEX_INITIALIZER; // utiliser pour ajouter des joueurs à une partie

// Pour compiler : gcc -DMAC serveur.c -o serveur
typedef struct joueur{
    int sock;
    int id; // entre 0 et 3
    int pret;
} joueur;

typedef struct partie {
    joueur * joueurs[4]; // les équipes sont faites en fonction de la parité
    int port_multi; // port de multidiffusion
    char * addr_multi; // adresse de multidiffusion FF12:
    int port; // port pour recevoir des messages
    int equipes; // vaut 1 pour une partie en équipe et 0 sinon
} partie;

typedef struct arg_serve{
    partie * partie4v4; // les parties qui ont encore de la place -> ajouter partie2v2
    partie * partie2v2;
    int sock;
} arg_serve;


joueur * nouveau_joueur(int sock, int i){
    joueur * res = malloc(sizeof(joueur));
    res->sock = sock;
    res->id = i;
    res->pret = 0;
    return res;
}


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

void *serve(void *arg) { // mettre des limites d'attente sur les recv
    arg_serve a = *((arg_serve *)arg);
    int sock = a.sock;
  
  // ** recevoir premier message : type de partie **

    //on reçoit la struct sous forme de string, on la reconvertit en struct et on la copie dans mess_client
    message_debut_client * mess_client = malloc(sizeof(message_debut_client));  // Je sais pas si ça marche, à tester avec le client ------------ normalement ça marche, on croise les doigts 
    memset(mess_client, 0, sizeof(*mess_client));

    char buf[sizeof(mess_client)];
    memset(buf, 0, sizeof(buf));
    int recu = 0;
    while(recu<sizeof(mess_client)) { 
        int r = recv(sock, buf+recu, sizeof(message_debut_client), 0);
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

    //printf("recu : %s\n", buf); // n'affichera pas grand chose 
    memcpy(mess_client,(message_debut_client*)&buf,sizeof(message_debut_client));
    uint16_t codereq_id_eq = ntohs(mess_client->CODEREQ_IQ_EQ);
    uint16_t codereq = codereq_id_eq & 0b1111111111111; // pour lire 13 bits
    // pour récupérer id -> on décalle de 13 dans l'autre sens ( >>13) & 0b11
    printf("codereq: %d \n",codereq);

    joueur * j;
    if (codereq==1) {
        // ajoute le joueur à une partie 4v4
        j = ajoute_joueur(a.partie4v4, sock);
        if (j==NULL){
            printf("Erreur le joueur n'a pas pu être ajouté\n");
            close(sock);
            free(arg);
            return NULL;
        }
    } else if (codereq==2){
        // ajoute le joueur à une partie2v2
        j = ajoute_joueur(a.partie2v2, sock);
        if (j==NULL){
            printf("Erreur le joueur n'a pas pu être ajouté\n");
            close(sock);
            free(arg);
            return NULL;
        }
    } else {
        printf("Valeur de codereq incorrecte\n");
        close(sock);
        free(arg);
        return NULL;
    }
   // Remplissage de la struct
    message_debut_serveur * mess = malloc(sizeof(message_debut_serveur));
    memset(mess, 0, sizeof(message_debut_serveur));

    if (codereq==1) { // partie 4v4
        mess->CODEREQ_ID_EQ = htons((13<<j->id)|9);
        mess->PORTUDP = htons(a.partie4v4->port);
        mess->PORTMDIFF = htons(a.partie4v4->port_multi);
        //inet_pton(AF_INET6, a.partie4v4->addr_multi, &mess.ADRMDIFF ); // C'est OK ?
    } else { // partie2v2
        mess->CODEREQ_ID_EQ = htons((15<<(j->id)%2)|(13<<j->id)|10);
        mess->PORTUDP = htons(a.partie2v2->port);
        mess->PORTMDIFF = htons(a.partie2v2->port_multi);
        //inet_pton(AF_INET6, a.partie2v2->addr_multi, &mess.ADRMDIFF ); // C'est OK ?

    }
    char* serialized_msg = malloc(BUF_SIZE*sizeof(char));
    memcpy(serialized_msg,(char*)mess,sizeof(message_debut_serveur));
    int ecrit = 0;
    while (ecrit<sizeof(mess)){
        ecrit += send(sock, &mess + ecrit, sizeof(mess)-ecrit, 0); // ???
    }
    printf("envoye\n");
    free(mess);
    // attendre le message "prêt" du joueur

   // Je sais pas si ça marche, à tester avec le client ------------ normalement ça marche, on croise les doigts 
    memset(mess_client, 0, sizeof(*mess_client));

    memset(buf, 0, sizeof(buf));

    recu = 0;
    while(recu<sizeof(message_debut_client)) { 
        int r = recv(sock, buf+recu, sizeof(message_debut_client), 0);
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
     memcpy(mess_client,(message_debut_client*)&buf,sizeof(message_debut_client));

    printf("recu \n");

    // vérifier les infos du message reçu
    // codereq
    uint16_t v = ntohs(mess_client->CODEREQ_IQ_EQ);

    uint16_t codereq2 = v & 0b1111111111111;
    printf ("codereq2 = %d\n",codereq2);
    if (codereq2 != codereq+2) {
        printf("erreur valeur codereq dans message pret\n");
        close(sock);
        free(arg);
        return NULL;
    }
    // id
    uint16_t id = ( v >>13) & 0b11;
    if (id != j->id){
        printf("erreur valeur id dans message pret\n");
        close(sock);
        free(arg);
        return NULL;
    }
    // eq
    if (codereq==2){
        int eq = (v >> 13) & 0b1;
        if (eq != id%2) {
            printf("erreur valeur eq dans message pret\n");
            close(sock);
            free(arg);
            return NULL;
        }
    }

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
    partie * p4v4 = nouvelle_partie(0);
    partie * p2v2 = nouvelle_partie(1);

    while(1){
        // accepte un client
        struct sockaddr_in6 addrclient;
        socklen_t size=sizeof(addrclient);
    
        int sock_client = accept(sock, (struct sockaddr *) &addrclient, &size);

        if (sock_client >= 0) {
            arg_serve * arg = malloc(sizeof(arg_serve));
            arg->partie4v4 = p4v4;
            arg->partie2v2 = p2v2;
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