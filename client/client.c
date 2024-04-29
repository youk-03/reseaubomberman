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
#define ADDR_TCP  "::1" //"fdc7:9dd5:2c66:be86:7e57:58ff:fe68:b249"  // "::1" //à changer
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

int ready_req(message_debut_client* msg_client, int mode, int id, int team) {
  msg_client->CODEREQ_IQ_EQ = htons((team<<15) | (id<<13) | (mode));
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

    message_debut_client * start_msg = malloc(sizeof(message_debut_client)); 

    if (start_msg == NULL) {
      perror("erreur de malloc");
      close(sock_tcp);
      return 1 ;
    }
    memset(start_msg, 0, sizeof(message_debut_client));
    join_req(start_msg,mode);

    char* serialized_msg = malloc(BUF_SIZE*sizeof(char));

     if (serialized_msg == NULL) {
      perror("erreur de malloc");
      close(sock_tcp);
      return 1 ;
    }

    memcpy(serialized_msg,start_msg,sizeof(&start_msg)); //
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

    memset(serv_msg, 0, sizeof(message_debut_serveur));
    memcpy(serv_msg,serialized_serv_msg,sizeof(message_debut_serveur));

    //todo: rajouter if équipes 

    //c'est à partir de serv_msg qu'on récupère les données envoyées par le serveur
        //  printf("codereq_id : %u \n",ntohs(serv_msg->CODEREQ_ID_EQ));  => garder ces valeurs quelque part
        //  printf("portmdiff : %u \n",ntohs(serv_msg->PORTMDIFF));
        //  printf("portupd : %u \n",ntohs(serv_msg->PORTUDP)); 
        struct in6_addr adrmdiff_convert;

        memset(&adrmdiff_convert,0,sizeof(struct in6_addr));
        memcpy(&adrmdiff_convert,serv_msg->ADRMDIFF,8*sizeof(uint16_t));
        char adrmdiff_string[INET6_ADDRSTRLEN];

        inet_ntop(AF_INET6,&adrmdiff_convert,adrmdiff_string,INET6_ADDRSTRLEN);
          
        printf("adresse de multidiffusion : %s \n",adrmdiff_string);

        //  //printf("eq %u\n",ntohs(serv_msg->CODEREQ_ID_EQ)>>15 & 0b1); //? 
        //  printf("id %u\n",ntohs(serv_msg->CODEREQ_ID_EQ)>>13 & 0b11);
        //  printf("codereq %u\n",ntohs(serv_msg->CODEREQ_ID_EQ) & 0b11111111111111);
         mode=ntohs(serv_msg->CODEREQ_ID_EQ) & 0b1111111111111; 
         if (mode==9) {
          mode=3;
         } else if (mode==10) {
          mode=4;
         } else {
          perror("erreur réception codereq"); //todo: améliorer gestion d'erreur
         }
         id=ntohs(serv_msg->CODEREQ_ID_EQ)>>13 & 0b11;
         if (id<0 || id>3) {
          perror ("erreur réception id");
         }
         //todo: s'assurer que les valeurs sont ok 
        

    
    //là, on suppose que le client a reçu l'adresse de multidiffusion


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


    int ifindex = if_nametoindex ("en0"); //en0 pour mac eth0 sinon
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
    ready_req(start_msg,mode,id,team);
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

    puts("send ready effectue (cli)");

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
  int i = send_req();
  return i;
}