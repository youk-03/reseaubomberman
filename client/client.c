#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "../format_messages.h"

#define PORT_TCP  1024
#define ADDR_TCP "fdc7:9dd5:2c66:be86:7e57:58ff:fe68:afa1" //à changer
#define BUF_SIZE 256


// peut-être pas
// int join_req(message_debut_client* msg_client, int mode) {// à bouger vers un autre fichier
//     msg_client->CODEREQ=htons(mode);
// }

// int ready_req(message_debut_client* msg_client, int team_id, int player_id) {
//   msg_client->EQ=htons(team_id);
//   msg_client->ID=htons(player_id);
// }

int join_req(message_debut_client* msg_client, int mode) { //1 si solo, 2 si équipes,à bouger vers un autre fichier
  msg_client->CODEREQ_IQ_EQ = htons(mode);
  return 0;
}

//mode est un peu redondant, je le remplacerai plus tard avec un |
int ready_req(message_debut_client* msg_client, int mode, int id, int team) {
  msg_client->CODEREQ_IQ_EQ = htons((15<<team) | (13<<id) | mode);
  return 0;
}


int send_req() {

  int mode = 1;
  int id = 3;
  int team = 2;
  
    /*Initialisations pour les communications en TCP*/

    int sock_tcp = socket(PF_INET6, SOCK_STREAM, 0);
    if (sock_tcp == -1) {
      perror ("erreur creation socket tcp client");
      return 1 ;
    }

    int conv ;
    struct sockaddr_in6 address_sock_tcp;
    memset(&address_sock_tcp, 0,sizeof(address_sock_tcp));
    address_sock_tcp.sin6_family = AF_INET6;
    address_sock_tcp.sin6_port = htons(PORT_TCP);
    conv = inet_pton(AF_INET6, ADDR_TCP, &address_sock_tcp.sin6_addr);
    if (conv != 1) {
      perror("erreur conversion adresse tcp");
    }

    int c = connect(sock_tcp, (struct sockaddr *) &address_sock_tcp, sizeof(address_sock_tcp)); 
    if (c == -1) {
      perror("erreur connexion tcp");
      close(sock_tcp);
      return 1 ;
    }

    /*envoi de la première "requête" */

    //todo: remplacer par un select/poll éventuellement

    message_debut_client * start_msg = malloc(sizeof(message_debut_client)); //verif
    memset(start_msg, 0, sizeof(message_debut_client));
    join_req(start_msg,mode);

    char* serialized_msg = malloc(BUF_SIZE*sizeof(char));
    memcpy(serialized_msg,(char*)start_msg,sizeof(start_msg)); //
    //todo: récuperer l'input de l'utilisateur

    
    //on passe le mode (pour tester: solo/1) dans la struct
    
    int s = 0;
  
    while (s < sizeof(serialized_msg)) {
      int sent = send(sock_tcp, serialized_msg + s, sizeof(serialized_msg) - s, 0);
      if (sent == -1) {
        perror("erreur de send");
        close(sock_tcp);
        return 1 ;
      }
      s += sent;
    }
 
    /*reception des données d'identification*/
    char* serialized_serv_msg = malloc(BUF_SIZE*sizeof(char));
    memset(serialized_serv_msg, 0, BUF_SIZE);


    int recu = recv(sock_tcp, serialized_serv_msg, sizeof(serialized_serv_msg), 0);
    if (recu < 0){
      perror("erreur reception donnees initiales (client)");
      close(sock_tcp);
      return 1;
    }
    puts("send start effectue (cli)");

    //printf("le client a reçu de la part du serveur : %s \n", bufrcv);

    //conversion du string en struct
    message_debut_serveur * serv_msg = malloc(sizeof(message_debut_serveur));
    memset(serv_msg, 0, sizeof(message_debut_serveur));
    memcpy(serv_msg,(message_debut_serveur*)serialized_serv_msg,sizeof(message_debut_serveur));
    //c'est à partir de serv_msg qu'on récupère les données envoyées par le serveur


    //là, on suppose que le client a reçu l'adresse de multidiffusion


    /*Initialisations pour les communications en UDP*/

    int sock_udp;
    if((sock_udp = socket(AF_INET6, SOCK_DGRAM, 0)) < 0) {
      perror("echec creation socket udp client");
      return 1;
    }

    int ok = 1;
    if(setsockopt(sock_udp, SOL_SOCKET, SO_REUSEADDR, &ok, sizeof(ok)) < 0) {
      perror("echec de SO_REUSEADDR (client)");
      close(sock_udp);
      return 1;
    }


    struct sockaddr_in6 addr_rcv_udp;
    memset(&addr_rcv_udp, 0, sizeof(addr_rcv_udp));
    addr_rcv_udp.sin6_family = AF_INET6;
    addr_rcv_udp.sin6_addr = in6addr_any;
    addr_rcv_udp.sin6_port = htons(11111); // à changer, dépend de la valeur envoyée par le serveur 
    socklen_t adrsize = sizeof(addr_rcv_udp);

    if(bind(sock_udp,(struct sockaddr*)&addr_rcv_udp,sizeof(addr_rcv_udp))) {
      perror("erreur de bind");
      close(sock_udp);
      return 1 ; 
    }


    int ifindex = if_nametoindex ("eth0");
    if(ifindex == 0)
      perror("if_nametoindex");

    // abonnement à l'adresse de multidiffusion

    struct ipv6_mreq group ;
    if(inet_pton(AF_INET6,"ff12::1:2:2",&group.ipv6mr_multiaddr.s6_addr)<1){ // à changer, dépend des données envoyées par le serveur
      perror("erreur conversion adresse");
      return 1;
    }
    group.ipv6mr_interface= ifindex;

    if (setsockopt(sock_udp, IPPROTO_IPV6, IPV6_JOIN_GROUP, &group, sizeof(group))<0) {
      perror ("erreur d'abonnement au groupe");
      close(sock_udp);
      return 1 ;
    }

    //remplissage de la struct avec les données obtenues
    memset(start_msg, 0, sizeof(message_debut_client));
    ready_req(start_msg,mode,id,team);

    //conversion de la struct en string
    char* serialized_ready_msg = malloc(BUF_SIZE*sizeof(char));
    memcpy(serialized_ready_msg,(char*)start_msg,sizeof(start_msg)); //

    s = 0;
    while (s < sizeof(serialized_ready_msg)) {
      int sent = send(sock_tcp, serialized_ready_msg + s, sizeof(serialized_ready_msg) - s, 0);
      if (sent == -1) {
        perror("erreur de send");
        close(sock_tcp);
        return 1 ;
      }
      s += sent;
    }

    puts("send ready effectue (cli)");


    free(serv_msg);
    free(start_msg);
    free(serialized_msg);
    free(serialized_serv_msg);
    free(serialized_ready_msg);
  


    return 0; // J'ai mis ça pour éviter le warning

}

int main(int argc, char *argv[]) {
  int i = send_req();
}