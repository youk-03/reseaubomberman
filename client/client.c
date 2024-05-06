#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "auxiliaire.h"

#define PORT_TCP  1024
#define ADDR_TCP  "::1" //"fdc7:9dd5:2c66:be86:7e57:58ff:fe68:b249" //à changer
#define BUF_SIZE 256


int send_req(int mode_input) {

  info_joueur * info_joueur = malloc(sizeof(info_joueur));

  if (info_joueur == NULL) {
    perror("erreur de malloc");
    return 1 ;
  }

  info_joueur->mode=mode_input;

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

    message_debut_client * start_msg = malloc(sizeof(message_debut_client)); 

    if (start_msg == NULL) {
      perror("erreur de malloc");
      close(sock_tcp);
      return 1 ;
    }

    char* serialized_msg = malloc(BUF_SIZE*sizeof(char));

     if (serialized_msg == NULL) {
      perror("erreur de malloc");
      close(sock_tcp);
      return 1 ;
    }

    // memset(start_msg, 0, sizeof(message_debut_client));
    // join_req(start_msg,mode);
    // memcpy(serialized_msg,start_msg,sizeof(&start_msg)); //
 

    struct_to_string(start_msg,serialized_msg,info_joueur->mode);

    
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

     if (serialized_serv_msg == NULL) {
      perror("erreur de malloc");
      close(sock_tcp);
      return 1 ;
    }

    int recu=0;
    while (recu<sizeof(message_debut_client)){
    int r = recv(sock_tcp, serialized_serv_msg+recu, sizeof(message_debut_serveur), 0);
    if (r < 0){
      perror("erreur reception donnees initiales (client)");
      close(sock_tcp);
      return 1;
    }
    recu+=r;
    }

    //conversion du string en struct
    message_debut_serveur * serv_msg = malloc(sizeof(message_debut_serveur));

     if (serv_msg == NULL) {
      perror("erreur de malloc");
      close(sock_tcp);
      return 1 ;
    }

    string_to_struct(serv_msg,serialized_serv_msg);

    //todo: rajouter if équipes 

    //c'est à partir de serv_msg qu'on récupère les données envoyées par le serveur

            // printf("codereq_id : %u \n",ntohs(serv_msg->CODEREQ_ID_EQ));
            // printf("portmdiff : %u \n",ntohs(serv_msg->PORTMDIFF));
            // printf("portupd : %u \n",ntohs(serv_msg->PORTUDP));

        struct in6_addr * adrmdiff_convert= malloc(sizeof(struct in6_addr));
        if (adrmdiff_convert == NULL) {
          perror("erreur de malloc");
          close(sock_tcp);
          return 1; 
        }

        set_addrdiff(adrmdiff_convert,serv_msg);

        char adrmdiff_string[INET6_ADDRSTRLEN];
        inet_ntop(AF_INET6,adrmdiff_convert,adrmdiff_string,INET6_ADDRSTRLEN);
          
        printf("adresse de multidiffusion : %s \n",adrmdiff_string);

        //  //printf("eq %u\n",ntohs(serv_msg->CODEREQ_ID_EQ)>>15 & 0b1); //? 
        //  printf("id %u\n",ntohs(serv_msg->CODEREQ_ID_EQ)>>13 & 0b11);
        //  printf("codereq %u\n",ntohs(serv_msg->CODEREQ_ID_EQ) & 0b11111111111111);
         if (info_check(info_joueur,serv_msg) != 0 ) {
          close(sock_tcp);
          return 1;
         }
      
    /*Initialisations pour les communications en UDP*/

    int sock_udp;
    if((sock_udp = socket(AF_INET6, SOCK_DGRAM, 0)) < 0) {
      perror("echec creation socket udp client");
      close(sock_tcp);
      return 1;
    }

    int ok = 1;
    if(setsockopt(sock_udp, SOL_SOCKET, SO_REUSEADDR, &ok, sizeof(ok)) < 0) {
      perror("echec de SO_REUSEADDR (client)");
      close(sock_udp);
      close(sock_tcp);
      return 1;
    }


    struct sockaddr_in6 addr_rcv_udp;
    memset(&addr_rcv_udp, 0, sizeof(addr_rcv_udp));
    addr_rcv_udp.sin6_family = AF_INET6;
    addr_rcv_udp.sin6_addr = in6addr_any;
    addr_rcv_udp.sin6_port = htons(ntohs(serv_msg->PORTUDP)); // port udp envoyé par le serveur
    //socklen_t adrsize = sizeof(addr_rcv_udp);

    if(bind(sock_udp,(struct sockaddr*)&addr_rcv_udp,sizeof(addr_rcv_udp))) {
      perror("erreur de bind");
      close(sock_udp);
      close(sock_tcp);
      return 1 ; 
    }


    int ifindex = if_nametoindex ("eth0"); //en0 pour mac eth0 sinon
    if(ifindex == 0)
      perror("if_nametoindex");

    // abonnement à l'adresse de multidiffusion

    struct ipv6_mreq group ;
    if(inet_pton(AF_INET6,adrmdiff_string,&group.ipv6mr_multiaddr.s6_addr)<1){ //dépend des données envoyées par le serveur
      perror("erreur conversion adresse");
      close(sock_tcp);
      close(sock_udp);
      return 1;
    }
    group.ipv6mr_interface= ifindex;

    if (setsockopt(sock_udp, IPPROTO_IPV6, IPV6_JOIN_GROUP, &group, sizeof(group))<0) {
      perror ("erreur d'abonnement au groupe");
      close(sock_udp);
      close(sock_tcp);
      return 1 ;
    }

    //remplissage de la struct avec les données obtenues
    memset(start_msg, 0, sizeof(message_debut_client));

    //conversion de la struct en string
    char* serialized_ready_msg = malloc(BUF_SIZE*sizeof(char));

     if (serialized_ready_msg == NULL) {
      perror("erreur de malloc");
      close(sock_tcp);
      close(sock_udp);
      return 1 ;
    }

    ready_req(start_msg,info_joueur);
    memcpy(serialized_ready_msg,start_msg,sizeof(&start_msg)); // 
    
    s = 0;
    while (s < sizeof(serialized_ready_msg)) {
      int sent = send(sock_tcp, serialized_ready_msg + s, sizeof(serialized_ready_msg) - s, 0);
      if (sent == -1) {
        perror("erreur de send");
        close(sock_tcp);
        close(sock_udp);
        return 1 ;
      }
      s += sent;
    }

    puts("send ready effectue (client)");

    // lecture de multidiffusion

    char buf[BUF_SIZE];
    memset(buf, 0, sizeof(buf));

    struct sockaddr_in6 diffadr;
    int multicast_recu;
    socklen_t difflen = sizeof(diffadr);

    memset(buf, 0, sizeof(buf));
    if ((multicast_recu = recvfrom(sock_udp, buf, sizeof(buf)-1, 0, (struct sockaddr *)&diffadr, &difflen)) < 0){
      perror("erreur de recvfrom");
      return -1;
    }
    printf("reçu en multidiffusion  : %s\n", buf);



    free(serv_msg);
    free(start_msg);
    free(serialized_msg);
    free(serialized_serv_msg);
    free(serialized_ready_msg);
  


    return 0; // J'ai mis ça pour éviter le warning

}

int main(int argc, char *argv[]) {

  if (argc < 2) {
    puts ("nombre d'arguments insuffisant");
    exit(0) ;
  }

  if(argv[1][0] != '1' && argv[1][0] != '2') {
    puts ("argument invalide");
    exit(0) ; 
  }

  int i = send_req(atoi(argv[1]));
  return i;
}