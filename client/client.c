#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <ncurses.h>
#include "auxiliaire.h"
#include "../game/myncurses.h"
#include "../game/game.h"

#define PORT_TCP  1024
#define ADDR  "::1" //"fdc7:9dd5:2c66:be86:7e57:58ff:fe68:b249" //à changer
#define BUF_SIZE 256

unsigned int num_msg = 0;


full_grid_msg* send_req(int mode_input, info_joueur* info_joueur, int *sock_udp, int *sock_mdiff, struct sockaddr_in6 *addr_udp) { //ALLOUER LES POINTEURS AVANT DE LES PASSER

  if (info_joueur == NULL) {
    perror("erreur de malloc");
    return NULL ;
  }

  info_joueur->mode=mode_input;

    /*Initialisations pour les communications en TCP*/

    int sock_tcp = socket(PF_INET6, SOCK_STREAM, 0);
    if (sock_tcp == -1) {
      perror ("erreur creation socket tcp client");
      return NULL ;
    }

    int conv ;
    struct sockaddr_in6 address_sock_tcp;
    memset(&address_sock_tcp, 0,sizeof(address_sock_tcp));
    address_sock_tcp.sin6_family = AF_INET6;
    address_sock_tcp.sin6_port = htons(PORT_TCP);
    conv = inet_pton(AF_INET6, ADDR, &address_sock_tcp.sin6_addr);
    if (conv != 1) {
      perror("erreur conversion adresse tcp");
    }

    int c = connect(sock_tcp, (struct sockaddr *) &address_sock_tcp, sizeof(address_sock_tcp)); 
    if (c == -1) {
      perror("erreur connexion tcp");
      close(sock_tcp);
      return NULL ;
    }

    /*envoi de la première "requête" */

    message_debut_client * start_msg = malloc(sizeof(message_debut_client)); 

    if (start_msg == NULL) {
      perror("erreur de malloc");
      close(sock_tcp);
      return NULL ;
    }

    char* serialized_msg = malloc(BUF_SIZE*sizeof(char));

     if (serialized_msg == NULL) {
      perror("erreur de malloc");
      close(sock_tcp);
      return NULL ;
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
        return NULL ;
      }
      s += sent;
    }
 
    /*reception des données d'identification*/
    char* serialized_serv_msg = malloc(BUF_SIZE*sizeof(char));
    memset(serialized_serv_msg, 0, BUF_SIZE);

     if (serialized_serv_msg == NULL) {
      perror("erreur de malloc");
      close(sock_tcp);
      return NULL ;
    }

    int recu=0;
    while (recu<sizeof(message_debut_client)){
    int r = recv(sock_tcp, serialized_serv_msg+recu, sizeof(message_debut_serveur), 0);
    if (r < 0){
      perror("erreur reception donnees initiales (client)");
      close(sock_tcp);
      return NULL;
    }
    recu+=r;
    }

    //conversion du string en struct
    message_debut_serveur * serv_msg = malloc(sizeof(message_debut_serveur));

     if (serv_msg == NULL) {
      perror("erreur de malloc");
      close(sock_tcp);
      return NULL ;
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
          return NULL; 
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
          return NULL;
         }
      
    /* Initialisation sock_udp */
    //////////////////////////////////////////////////////////////////////////


    if((*sock_udp = socket(AF_INET6, SOCK_DGRAM, 0)) < 0) {
      perror("echec creation socket udp client");
      close(sock_tcp);
      return NULL;
    }


    int ok = 1;
    if(setsockopt(*sock_udp, SOL_SOCKET, SO_REUSEADDR, &ok, sizeof(ok)) < 0) {
      perror("echec de SO_REUSEADDR (client)");
      close(*sock_udp);
      close(sock_tcp);
      return NULL;
    }


    //struct sockaddr_in6 addr_udp;
    addr_udp->sin6_family = AF_INET6;
    addr_udp->sin6_port = serv_msg->PORTUDP; // port udp envoyé par le serveur
    inet_pton(AF_INET6, ADDR, &addr_udp->sin6_addr);

    //////////////////////////////////////////////////////////////////////:

    /* Initialisation sock_mdiff */

    //int sock_mdiff;

    if((*sock_mdiff = socket(AF_INET6, SOCK_DGRAM, 0)) < 0) {
      perror("echec de socket");
      close(*sock_udp);
      close(sock_tcp);
      return NULL;
    }

    ok = 1;
    if(setsockopt(*sock_mdiff, SOL_SOCKET, SO_REUSEADDR, &ok, sizeof(ok)) < 0) {
      perror("echec de SO_REUSEADDR");
      close(*sock_mdiff);
      close(*sock_udp);
      close(sock_tcp);
      return NULL;
    }

    struct sockaddr_in6 addr_mdiff;
    memset(&addr_mdiff, 0, sizeof(addr_mdiff));
    addr_mdiff.sin6_family = AF_INET6;
    addr_mdiff.sin6_addr = in6addr_any;
    addr_mdiff.sin6_port = serv_msg->PORTMDIFF;

    if(bind(*sock_mdiff, (struct sockaddr*) &addr_mdiff, sizeof(addr_mdiff))) {
      perror("echec de bind");
      close(*sock_mdiff);
      close(*sock_udp);
      close(sock_tcp);
      return NULL;
    }

    int ifindex = if_nametoindex ("eth0");
    if(ifindex == 0)
      perror("if_nametoindex");



    // abonnement à l'adresse de multidiffusion

    struct ipv6_mreq group ;
    if(inet_pton(AF_INET6,adrmdiff_string,&group.ipv6mr_multiaddr.s6_addr)<1){ //dépend des données envoyées par le serveur
      perror("erreur conversion adresse");
      close(*sock_mdiff);
      close(sock_tcp);
      close(*sock_udp);
      return NULL;
    }
    group.ipv6mr_interface= ifindex;

    if (setsockopt(*sock_udp, IPPROTO_IPV6, IPV6_JOIN_GROUP, &group, sizeof(group))<0) {
      perror ("erreur d'abonnement au groupe");
      close(*sock_mdiff);
      close(*sock_udp);
      close(sock_tcp);
      return NULL ;
    }
    ////////////////////////////////////////////////////////////////////////

    //remplissage de la struct avec les données obtenues
    memset(start_msg, 0, sizeof(message_debut_client));

    //conversion de la struct en string
    char* serialized_ready_msg = malloc(BUF_SIZE*sizeof(char));

     if (serialized_ready_msg == NULL) {
      perror("erreur de malloc");
      close(*sock_mdiff);
      close(sock_tcp);
      close(*sock_udp);
      return NULL ;
    }

    ready_req(start_msg,info_joueur);
    memcpy(serialized_ready_msg,start_msg,sizeof(&start_msg)); // 
    
    s = 0;
    while (s < sizeof(serialized_ready_msg)) {
      int sent = send(sock_tcp, serialized_ready_msg + s, sizeof(serialized_ready_msg) - s, 0);
      if (sent == -1) {
        perror("erreur de send");
        close(*sock_mdiff);
        close(sock_tcp);
        close(*sock_udp);
        return NULL ;
      }
      s += sent;
    }

    puts("send ready effectue (client)");

    // lecture de multidiffusion

    char *buf= malloc(sizeof(full_grid_msg));
    full_grid_msg *msg = malloc(sizeof(full_grid_msg)); 


    struct sockaddr_in6 diffadr;
    int multicast_recu;
    socklen_t difflen = sizeof(diffadr);

    memset(buf, 0, sizeof(buf));
    if ((multicast_recu = recvfrom(*sock_mdiff, buf, sizeof(full_grid_msg), 0, (struct sockaddr *)&diffadr, &difflen)) < 0){
      perror("erreur de recvfrom");
      return NULL;
    }
    memcpy(msg, buf, sizeof(full_grid_msg));
    printf("reçu en multidiffusion  : %s\n", buf);


    free(buf);
    buf=NULL;
    free(serv_msg);
    free(start_msg);
    free(serialized_msg);
    free(serialized_serv_msg);
    free(serialized_ready_msg);
  


    return msg; 

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
  //stock info du joueur (id etc)
  info_joueur * info_joueur = malloc(sizeof(info_joueur));

  int *sock_udp = malloc(sizeof(int));
  int *sock_mdiff = malloc(sizeof(int));
  struct sockaddr_in6 *serv_addr = malloc(sizeof(struct sockaddr_in6));
  memset(serv_addr, 0, sizeof(struct sockaddr_in6));

  full_grid_msg* init_grid = send_req(atoi(argv[1]), info_joueur, sock_udp, sock_mdiff, serv_addr);
  if(!init_grid){
    dprintf(2,"erreur lors de l'envoi de requete exit");
    close(*sock_mdiff);
    close(*sock_udp);
    free(sock_udp);
    free(sock_mdiff);
    exit(1);
  }
  printf("envoie grille effectue\n");
  //id joueur 0-3 enum val pareil

  int id_joueur = info_joueur->id;
  board *board = malloc(sizeof(board)); 
  pos *pos = malloc(sizeof(pos)); 

  //initialise sa pos en fonction de qui il est 
  switch(id_joueur){
    case 0: pos->x = 1; pos->y = 1; break;//joueur 1
    case 1: pos->x = board->w-1; pos->y = 1; break;//joueur 2
    case 2: pos->x = 1; pos->y = board->h-1; break;//joueur 3
    case 3: pos->x = board->w-1; pos->y = board->h-1; break;//joueur 4
  }


  setup_board(board);//initialise la grid

  maj_grid(init_grid,board); //passage de req vers la grid pour l'affichage

  //setup bibliotheque ncurses
    line* l = malloc(sizeof(line));//utile ?
    l->cursor = 0;

    // NOTE: All ncurses operations (getch, mvaddch, refresh, etc.) must be done on the same thread.
    initscr(); /* Start curses mode */ // Initialise la structure WINDOW et autres paramètres
    raw(); /* Disable line buffering */
    intrflush(stdscr, FALSE); /* No need to flush when intr key is pressed */
    keypad(stdscr, TRUE); /* Required in order to get events from keyboard */
    nodelay(stdscr, TRUE); /* Make getch non-blocking */
    noecho(); /* Don't echo() while we do getch (we will manually print characters when relevant) */
    curs_set(0); // Set the cursor to invisible
    start_color(); // Enable colors
    init_pair(1, COLOR_YELLOW, COLOR_BLACK); // Define a new color style (text is yellow, background is black)
    init_pair(2, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(3, COLOR_RED, COLOR_BLACK);

    refresh_game(board,NULL); //affiche board

     //int tick = 30*1000;
     bomblist *list =create_list(10);
     message_partie_client *msg;

     int env=-2;
     char *buffer = malloc(sizeof(message_partie_client)+1);
     memset(buffer,0,sizeof(message_partie_client)+1);


    while (true) {
        ACTION a = control(l);

        if (msg = perform_action_req(board, pos, a, list,info_joueur, num_msg)){ //joueur demande a quitter le jeu si NULL
        num_msg++;
        num_msg = num_msg%8192; //2^13

        memcpy(buffer,msg,sizeof(message_partie_client));


          for(int i=0; i<sizeof(message_partie_client); i++){
        printf("%02x", buffer[i]);
    }   

        //envoyer la struct au serveur en udp
        env = sendto(*sock_udp, buffer, sizeof(message_partie_client), 0, (struct sockaddr *) serv_addr, sizeof(struct sockaddr_in6));

        if(env<0){
          perror("echec sendto client game");
          close(*sock_mdiff);
          close(*sock_udp);
          free(sock_udp);
          free(sock_mdiff);
          free(buffer);
          free(init_grid);
          free(info_joueur);
          exit(0);
        }
        exit(0);
           //printf("COUCOUu\n");
        memset(buffer,0,sizeof(message_partie_client)+1);
        memset(msg,0,sizeof(message_partie_client));



        //recevoir un message du serveur en udp

        //avec poll donc déclarer 2 socket udp

        //maj la grid en fonction du message 
        //gerer en parallele l'envoie des req et la reception des req ? pour qu'elle se fasse en même temps (avec poll ?)

        //refresh_game(board,l);
        //usleep(tick);
        //if(maj_bomb(list,tick,board)) break;
        free(msg);/////////////////////

        }

    }

    printf("gameover");
    free_board(board);
    free(pos);
    free(l);
    empty_list(list);

   curs_set(1); // Set the cursor to visible again
   endwin(); /* End curses mode */


  //affichage de la grille pour le client
  //dans info joueur id initialisé

      //         for(int i=0; i<sizeof(full_grid_msg); i++){
    //     printf("%02x", buf[i]);
    // }

    free(init_grid);
    close(*sock_mdiff);
    close(*sock_udp);
    free(sock_udp);
    free(sock_mdiff);
    free(buffer);
    free(info_joueur);
    free(msg);
  return 0;

  //rajouter le jeux, la grille avec le board toussa toussa et faire en sorte que quand le 
  //client essaye de se deplacer au lieu de changer la grille ça envoie une requête
  //au serveur
  //récupérer les grille multidiffusé, maj la grille du client et afficher

  //le client stock qui il est normalement ? (id joueur)



  /* pour le serveur :
  le serveur va avoir la grille la vrai grille, il va recevoir les demande des joueurs les maj
  puis envoyer au joueur en multidiffusion la grille modifiée et toute les secondes la grille complete
  
  coder les fonc qui recupere les requetes des joueurs et maj la grid avec */
}