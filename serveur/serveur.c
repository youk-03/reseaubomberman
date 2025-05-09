#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include "../format_messages.h"
#include "partie.h"
#include "joueur.h"

#ifdef MAC
#ifdef SO_REUSEADDR
#undef SO_REUSEADDR
#endif
#define SO_REUSEADDR SO_REUSEPORT
#endif

int main(int argc, char *argv[]){
    // *** Recevoir les messages de nouveaux joueurs ***
    init_socks();

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

    partie * p4v4 = nouvelle_partie(0); 
    partie * p2v2 = nouvelle_partie(1); 

    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = termine;
    if (sigaction(SIGINT, &action, NULL)==-1) {
        perror("erreur sigaction");
    }

    while(1){

        // accepte un client
        struct sockaddr_in6 addrclient;
        socklen_t size=sizeof(addrclient);
    
        int sock_client = accept(sock, (struct sockaddr *) &addrclient, &size); 
        ajoute_client(sock_client);

        if (sock_client >= 0) {
            arg_serve * arg = malloc(sizeof(arg_serve));
            arg->partie4v4 = &p4v4; 
            arg->partie2v2 = &p2v2;
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

    }

    close(sock);
    return 0;



}