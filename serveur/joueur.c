#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "joueur.h"
#include "partie.h"
#include "../format_messages.h"



joueur * nouveau_joueur(int sock, int i){
    joueur * res = malloc(sizeof(joueur));
    res->sock = sock;
    res->id = i;
    res->pret = 0;
    return res;
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

        //  printf("codereq_id : %u \n",mess->CODEREQ_ID_EQ);
        //  printf("portmdiff : %u \n",mess->PORTMDIFF);
        //  printf("portupd : %u \n",mess->PORTUDP);

    char* serialized_msg = malloc(BUF_SIZE*sizeof(char));
    memcpy(serialized_msg,(char*)mess,sizeof(message_debut_serveur));

    int ecrit = 0;  
    while (ecrit<sizeof(serialized_msg)){
        ecrit += send(sock, serialized_msg + ecrit, sizeof(serialized_msg)-ecrit, 0); // ??? -> on cast la struct en char* et on envoie le char* du résultat
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