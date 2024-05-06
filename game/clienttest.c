#include "game.h"
#include "myncurses.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <net/if.h>
#include "../format_messages.h" 

//PUREMENT POUR DES TESTS NE RIEN FAIRE SVP

// int main(int argc, char* argv[]){
//    int sock= socket(PF_INET6, SOCK_DGRAM, 0);
//    if (sock < 0) return -1;
    
//     struct sockaddr_in6 servadr;
//     memset(&servadr, 0, sizeof(servadr));
//     servadr.sin6_family = AF_INET6;
//     servadr.sin6_addr = in6addr_any;
//     servadr.sin6_port = htons(2525);
//     //socklen_t len = sizeof(servadr);

//     if(bind(sock, (struct sockaddr *) &servadr, sizeof(servadr))<0) return -1;

//     char *buffer= malloc(sizeof(full_grid_msg));
//     full_grid_msg *msg = malloc(sizeof(full_grid_msg));
//     //modified_cases_msg *msg = malloc(sizeof(modified_cases_msg));
//     memset(msg,0, sizeof(full_grid_msg));
//     //memset(msg,0, sizeof(modified_cases_msg));

//     int r = recvfrom(sock, buffer, sizeof(full_grid_msg),0, NULL, NULL);
//     if(r<0) return -2;    
//     memcpy(msg, buffer, sizeof(full_grid_msg));

//         for(int i=0; i<sizeof(full_grid_msg); i++){
//         printf("%02x", buffer[i]);
//     }

//     uint8_t hauteur = ntohs(msg->HAUTEUR_LARGEUR) & 0b11111111;
//     uint8_t largeur = ntohs(msg->HAUTEUR_LARGEUR) >> 8 & 0b11111111;

//     printf("\n codereqideq %u, hauteur %u largeur %u, num %u \n ", ntohs(msg->CODEREQ_ID_EQ),hauteur, largeur, ntohs(msg->NUM));

//     //refresh_game(b, NULL);

//     // printf("NUM DE LA STRUCT %u----\n", ntohs(msg->NUM));
//     // printf("NUM DE LA STRUCT %s----", buffer);

// }

//fonction qui prend un full grid_msg et le board et le modifie en fonction du full_grid msg