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

void *serve(void *arg) { 
    arg_serve * a = ((arg_serve * )arg);
    int sock = a->sock;
  
  // ** recevoir premier message : type de partie **

    //on reçoit la struct sous forme de string, on la reconvertit en struct et on la copie dans mess_client
    message_debut_client * mess_client = malloc(sizeof(message_debut_client));  
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


    memcpy(mess_client,(message_debut_client*)&buf,sizeof(message_debut_client));
    uint16_t codereq_id_eq = ntohs(mess_client->CODEREQ_ID_EQ);
    uint16_t codereq = codereq_id_eq & 0b1111111111111; 

    joueur * j;
    if (codereq==1) {
        // ajoute le joueur à une partie 4v4

        j = ajoute_joueur(a->partie4v4, sock);
        if (j==NULL){
            printf("Erreur le joueur n'a pas pu être ajouté\n");
            close(sock);
            free(mess_client);
            free(arg);
            return NULL;
        }
    } else if (codereq==2){       
        // ajoute le joueur à une partie2v2
        j = ajoute_joueur(a->partie2v2, sock);
        if (j==NULL){
            printf("Erreur le joueur n'a pas pu être ajouté\n");
            close(sock);
            free(mess_client);
            free(arg);
            return NULL;
        }
    } else {
        printf("Valeur de codereq incorrecte\n");
        close(sock);
        free(arg);
        free(mess_client);
        return NULL;
    }
   // Remplissage de la struct
    message_debut_serveur * mess = malloc(sizeof(message_debut_serveur));
    memset(mess, 0, sizeof(message_debut_serveur));

    if (codereq==1) { // partie 4v4
  
        mess->CODEREQ_ID_EQ = htons((j->id)<<13|9);
        partie * p4v4 = * (a->partie4v4);
        mess->PORTUDP = htons(p4v4->port);
        mess->PORTMDIFF = htons(p4v4->port_multi);
        if (inet_pton(AF_INET6, p4v4->addr_multi, mess->ADRMDIFF )!=1){
            perror("erreur inet_pton");
        }; 

    } else { // partie2v2
        mess->CODEREQ_ID_EQ = htons(((j->id)%2)<<15|(j->id<<13)|10); 
        partie * p2v2 = * (a->partie2v2);
        mess->PORTUDP = htons(p2v2->port);
        mess->PORTMDIFF = htons(p2v2->port_multi);
        inet_pton(AF_INET6, p2v2->addr_multi, mess->ADRMDIFF ); 

    }

   
    char* serialized_msg = malloc(sizeof(message_debut_serveur));
    memset(serialized_msg, 0, sizeof(message_debut_serveur));
    memcpy(serialized_msg, mess,sizeof(message_debut_serveur));

    int ecrit = 0;  
    while (ecrit<sizeof(message_debut_serveur)){
        ecrit += send(sock, serialized_msg + ecrit, sizeof(message_debut_serveur)-ecrit, 0); 
    }   
    free(mess);
    free(serialized_msg);

    memset(mess_client, 0, sizeof(*mess_client));

    memset(buf, 0, sizeof(buf));

    recu = 0;
    while(recu<sizeof(message_debut_client)) { 
        int r = recv(sock, buf+recu, sizeof(message_debut_client), 0);
        if (r<0){
            perror("recv");
            close(sock);
            free(arg);
            return NULL;
        }
        recu += r;
    }
     memcpy(mess_client,(message_debut_client*)&buf,sizeof(message_debut_client));


    // vérifier les infos du message reçu
    // codereq
    uint16_t v = ntohs(mess_client->CODEREQ_ID_EQ);

    free(mess_client);
  
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

    j->pret = 1 ;

    free(arg);
    return NULL;
}