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
#include "game.h"
#include "myncurses.h"
#include <bits/socket.h>
#define BITSOF(x) (sizeof(x) * 8)


void print_uint8_t(uint8_t n) {
        int i;
        for (i = BITSOF(n); i >= 0; i--)
                printf("%d", (n & (1<<i)) >> i);
        putchar('\n');
}

modified_cases_msg* modified_grid_req(unsigned int num, uint8_t nb){
    modified_cases_msg *res = malloc(sizeof(modified_cases_msg));
    if(!res){
        exit(1); //sur ?
    }
    res->CODEREQ_ID_EQ = htons(12);
    res->NUM = htons(num);
    res->NB = nb;

   // print_uint8_t(res->NB);

    return res;
}



full_grid_msg* full_grid_req (board *b, unsigned int num){
    full_grid_msg *res = malloc(sizeof(full_grid_msg));
    if(!res){
        exit(1); //sur ?
    }
    res->CODEREQ_ID_EQ = htons(11);
    res->NUM = htons(num);
    res->HAUTEUR_LARGEUR = htons((b->w)<<8| b->h);
    //case
    __uint8_t casei=0;
    for(int i = 0; i<GRID_SIZE; i++){ //hum pas sur pour la taille vu que le machin fait des bonds de 2
        switch(b->grid[i]){
            case EMPTY: casei = 0; break;
            case CHARACTER: casei=5; break;
            case CHARACTER2: casei=6; break;
            case CHARACTER3: casei=7; break;
            case CHARACTER4: casei=8; break;
            case BWALL: casei= 2; break;
            case WALL: casei = 1; break;
            case BOMB: casei = 3; break;
        }
        res->CASE[i] = casei;
    }

    return res;
}

//fonction qui prend un full grid_msg et le board et le modifie en fonction du full_grid msg

void maj_grid (full_grid_msg *msg, board *b){
    int i=0;
    for(int y=0; y< b->h; y++){
        for(int x = 0; x<b->w; x++){
            switch(msg->CASE[i]){
                case 0: set_grid(b,x,y, EMPTY); break;
                case 5: set_grid(b,x,y, CHARACTER); break;
                case 6: set_grid(b,x,y, CHARACTER2); break;
                case 7: set_grid(b,x,y, CHARACTER3); break;
                case 8: set_grid(b,x,y, CHARACTER4); break;
                case 2: set_grid(b,x,y, BWALL); break;
                case 1: set_grid(b,x,y, WALL); break;
                case 3: set_grid(b,x,y, BOMB); break;
            }

            i++;

        }
    }
}

// int main (int argc, char* argv[]){
//     //test udp ipv6
//     int sock = socket(PF_INET6, SOCK_DGRAM, 0);
//     if(sock < 0){
//         perror("erreur socket: ");
//         return -1;
//     }

//     struct sockaddr_in6 servadr;
//     memset(&servadr, 0, sizeof(servadr));
//     servadr.sin6_family = AF_INET6;
//     inet_pton(AF_INET6, "::1", &servadr.sin6_addr); //addr en arg svp
//     servadr.sin6_port = htons(2525);
//     socklen_t len = sizeof(servadr);

//     //envoie du message
//     board* b = malloc(sizeof(board));;
//     setup_board(b);
//     full_grid_msg *req =full_grid_req(b, 5);
//     char *buffer = malloc(sizeof(full_grid_msg)+1);
//     memcpy(buffer,req,sizeof(full_grid_msg));

//     for(int i=0; i<sizeof(full_grid_msg); i++){
//         printf("%02x", buffer[i]);
//     }

//     //tester une fonc pour print la grid ?

//     //refresh_game(b, NULL);


//     // modified_cases_msg *req =modified_grid_req(3,0x5);
//     // char *buffer = malloc(sizeof(modified_cases_msg)+1);
//     // memcpy(buffer,req,sizeof(modified_cases_msg));

//     // for(int i=0; i<sizeof(modified_cases_msg); i++){
//     //     printf("%02x", buffer[i]);
//     // }

//     // printf("\n");
//     // printf("%u, %u num %u l'autre", ntohs(req->NUM), ntohs(req->CODEREQ_ID_EQ));



//     int env = sendto(sock, buffer, sizeof(full_grid_msg), 0, (struct sockaddr *)&servadr, len);
//     if(env < 0){
//         perror("echec sendto");
//         return -2;
//     }


//     return 0;
// }