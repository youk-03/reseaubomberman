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
#include "../client/auxiliaire.h"
#include "game.h"
#include "myncurses.h"
#include "../serveur/partie.h"
#include <bits/socket.h>
#define BITSOF(x) (sizeof(x) * 8)


//stocker les requete udp des joueurs pour les appliquer (dans un tableau)


message_partie_client_liste* create_list_msg (int capacity){
    message_partie_client_liste* res = malloc(sizeof(message_partie_client));
    if(!res) goto error;

    res->capacity = capacity;
    res->length = 0;
    res->msg_liste = malloc(sizeof(message_partie_client*)*capacity);

    if(!res->msg_liste) goto error;

    return res;
    error:
    exit(1);
}

void add_liste_msg (message_partie_client_liste *list, message_partie_client* data){

    if(list->capacity == list->length){
        list->capacity = list->capacity*2;
        list->msg_liste = realloc(list->msg_liste, sizeof(message_partie_client)*list->capacity);
        if(!list->msg_liste){
            goto error;
        
        }
    }

    list->msg_liste[list->length] = data; 
    list->length++;

    return;

    error:
    exit(1);
}

int remove_msg(message_partie_client_liste *list, message_partie_client *data){ 

    int i = 0;
    bool deleted = false;

    for(i; i<list->length; i++){
        if(list->msg_liste[i] == data){
            free(list->msg_liste[i]);
            deleted = true;
            break;
        }

    }

    if(deleted){
        memmove(list->msg_liste+i,list->msg_liste+i+1,(list->length-i-1)*sizeof(message_partie_client));
        list->length--;
    }
    else{
        return -1;
    }

    return 0;
}


void free_list(message_partie_client_liste *list){

    for(int i=0; i<list->length; i++){
       free(list->msg_liste[i]);
    }

    free(list->msg_liste);
    list->msg_liste = NULL;
    free(list);
    list=NULL;
}

void empty_list_msg(message_partie_client_liste *list){

    for(int i=0; i<list->length; i++){
       free(list->msg_liste[i]);
       list->msg_liste[i] = NULL;
    }

    list->length=0;

}

//caseholder

caseholder* create_list_caseholder (int capacity){
    caseholder* res = malloc(sizeof(caseholder));
    if(!res) goto error;

    res->capacity = capacity;
    res->length = 0;
    res->caseh = malloc(sizeof(CASES*)*capacity);

    if(!res->caseh) goto error;

    return res;
    error:
    exit(1);
}

void add_liste_case (caseholder *list, CASES* data){

    if(list->capacity == list->length){
        list->capacity = list->capacity*2;
        list->caseh = realloc(list->caseh, sizeof(CASES)*list->capacity);
        if(!list->caseh){
            goto error;
        
        }
    }

    list->caseh[list->length] = data; 
    list->length++;

    return;

    error:
    exit(1);
}

int remove_case(caseholder *list, CASES *data){ 

    int i = 0;
    bool deleted = false;

    for(i; i<list->length; i++){
        if(list->caseh[i] == data){
            free(list->caseh[i]);
            deleted = true;
            break;
        }

    }

    if(deleted){
        memmove(list->caseh+i,list->caseh+i+1,(list->length-i-1)*sizeof(CASES));
        list->length--;
    }
    else{
        return -1;
    }

    return 0;
}


void free_caseholder(caseholder *list){

    for(int i=0; i<list->length; i++){
       free(list->caseh[i]);
    }

    free(list->caseh);
    list->caseh = NULL;
    free(list);
    list=NULL;
}





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
            case EXPLODE: casei = 4; break;
        }
        res->CASE[i] = casei;
    }

    return res;
}

int from_gtor(int c){
    int casei = -1;

    switch(c){
        case EMPTY: casei = 0; break;
        case CHARACTER: casei=5; break;
        case CHARACTER2: casei=6; break;
        case CHARACTER3: casei=7; break;
        case CHARACTER4: casei=8; break;
        case BWALL: casei= 2; break;
        case WALL: casei = 1; break;
        case BOMB: casei = 3; break;
        case EXPLODE: casei = 4; break;
    }

    return casei;

}

int from_rtog (int c){

    int res= -1;

    switch(c){
        case 0: res = EMPTY; break;
        case 5: res =CHARACTER; break;
        case 6: res =CHARACTER2; break;
        case 7: res =CHARACTER3; break;
        case 8: res =CHARACTER4; break;
        case 2 : res = BWALL; break;
        case 1 : res = WALL; break;
        case 3 : res = BOMB; break;
        case 4 : res = EXPLODE; break;

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
                case 4: set_grid(b,x,y, EXPLODE); break;
            }

            i++;

        }
    }
}

void maj_grid_cases (caseholder *c, board *b){
    int x = -1;
    int y = -1;
    int obj = -1;

    for(int i=0; i<c->length; i++){
        dprintf(2,"bufcaseholderseg\n");
        y= ntohs(c->caseh[i]->xy) >> 8 & 0b11111111;
        x= ntohs(c->caseh[i]->xy) & 0b11111111;
        obj = from_rtog(c->caseh[i]->action);
        set_grid(b,x,y,obj);
    }
}

void copy_board(board *old, board* new){

    for(int y=0; y< old->h; y++){

        for(int x = 0; x<old->w; x++){

            set_grid(old,x,y,get_grid(new,x,y));

     }

    }

}

caseholder* get_difference(board *old, board *new){
    caseholder *res = create_list_caseholder(10);
    CASES *cases;
    int diff= 0;

        for(int y=0; y< old->h; y++){

            for(int x = 0; x<old->w; x++){
     
                if(get_grid(old,x,y) != get_grid(new,x,y)){
                      printf("board modif\n");
                    cases = malloc(sizeof(CASES));
                    cases->xy = htons(y << 8 | x);
                    cases->action = from_gtor(get_grid(new,x,y));
                     add_liste_case(res, cases);
                     diff++;
                }
                
            }

        }

        if(diff == 0){
            free_caseholder(res); return NULL;
        }

        return res;

}

modified_cases_msg* maj_board(message_partie_client_liste* list, bomblist *bomblist, partie *p, unsigned int num, bool **death){

    board *board = p->board;
    message_partie_client* req_j1[2]; //0 requete de deplacement ou quitter le jeu //1 requete de bombe
    req_j1[0] = NULL;
    req_j1[1] = NULL;
    message_partie_client* req_j2[2];
    req_j2[0] = NULL;
    req_j2[1] = NULL;
    message_partie_client* req_j3[2];
    req_j3[0] = NULL;
    req_j3[1] = NULL;
    message_partie_client* req_j4[2];
    req_j4[0] = NULL;
    req_j4[1] = NULL;
    int id = -1;
    int action =-1;
    unsigned int num_msg_cli = 0;


    for(int i=0; i<list->length; i++){

        id = ntohs(list->msg_liste[i]->CODEREQ_ID_EQ) >> 13 & 0b11;
        action = ntohs(list->msg_liste[i]->NUM_ACTION) >> 13 & 0b111;
        num_msg_cli = ntohs(list->msg_liste[i]->NUM_ACTION) & 0b1111111111111;

        switch(id){ //remove si pas implementee sur la grille
            case 0:
             if(action == 4){
                if(req_j1[1]== NULL){
                    req_j1[1] = list->msg_liste[i];
                }
                else{
                remove_msg(list, list->msg_liste[i]); i--; break;
                }
             }
             else {
                if(req_j1[0] == NULL){
                    req_j1[0] = list->msg_liste[i]; 
                }
                else if((ntohs(req_j1[0]->NUM_ACTION) & 0b1111111111111)< num_msg_cli){ //req plus recente donc
                    req_j1[0] = list->msg_liste[i]; 
                }
                else {
                    remove_msg(list, list->msg_liste[i]); i--; break;
                }
             }
             break;

            case 1:
             if(action == 4){
                if(req_j2[1]== NULL){
                    req_j2[1] = list->msg_liste[i];
                }
                else{
                remove_msg(list, list->msg_liste[i]); i--; break;
                }
             }
             else {
                if(req_j2[0] == NULL){
                    req_j2[0] = list->msg_liste[i]; 
                }
                else if((ntohs(req_j2[0]->NUM_ACTION) & 0b1111111111111)< num_msg_cli){ //req plus recente donc
                    req_j2[0] = list->msg_liste[i]; 
                }
                else {
                    remove_msg(list, list->msg_liste[i]); i--; break;
                }
             }
             break;
            case 2:
             if(action == 4){
                if(req_j3[1]== NULL){
                    req_j3[1] = list->msg_liste[i];
                }
                else{
                remove_msg(list, list->msg_liste[i]); i--; break;
                }
             }
             else {
                if(req_j3[0] == NULL){
                    req_j3[0] = list->msg_liste[i]; 
                }
                else if((ntohs(req_j3[0]->NUM_ACTION) & 0b1111111111111)< num_msg_cli){ //req plus recente donc
                    req_j3[0] = list->msg_liste[i]; 
                }
                else {
                    remove_msg(list, list->msg_liste[i]); i--; break;
                }
             }
             break;           
            case 3: 
             if(action == 4){
                if(req_j4[1]== NULL){
                    req_j4[1] = list->msg_liste[i];
                }
                else{
                remove_msg(list, list->msg_liste[i]); i--; break;
                }
             }
             else {
                if(req_j4[0] == NULL){
                    req_j4[0] = list->msg_liste[i]; 
                }
                else if((ntohs(req_j4[0]->NUM_ACTION) & 0b1111111111111)< num_msg_cli){ //req plus recente donc
                    req_j4[0] = list->msg_liste[i]; 
                }
                else {
                    remove_msg(list, list->msg_liste[i]); i--; break;
                }
             }
             break;
        }

    }

    //maj board

    from_clientreq_toboard(board, req_j1[0], p->joueurs[0]->pos, bomblist);
    from_clientreq_toboard(board, req_j1[1], p->joueurs[0]->pos, bomblist);   

    from_clientreq_toboard(board, req_j2[0], p->joueurs[1]->pos, bomblist);
    from_clientreq_toboard(board, req_j2[1], p->joueurs[1]->pos, bomblist);

    from_clientreq_toboard(board, req_j3[0], p->joueurs[2]->pos, bomblist);
    from_clientreq_toboard(board, req_j3[1], p->joueurs[2]->pos, bomblist);   

    from_clientreq_toboard(board, req_j4[0], p->joueurs[3]->pos, bomblist);
    from_clientreq_toboard(board, req_j4[1], p->joueurs[3]->pos, bomblist);

    //retourne enquete de modified_msg l'envoie des cases se fait en dehors  

    return modified_grid_req(num, list->length); 
 

    //VIDER LA LISTE MAINTENANT apres
    //empty_list_msg(list);

}

void from_clientreq_toboard(board *board, message_partie_client *msg, pos* pos, bomblist *bomblist ){
    if(msg == NULL){
        return;
    }


    int id = ntohs(msg->CODEREQ_ID_EQ) >> 13 & 0b11;
    int action =ntohs(msg->NUM_ACTION) >> 13 & 0b111;
    unsigned int num = ntohs(msg->NUM_ACTION) & 0b1111111111111;
    ACTION a;

    printf("%d id, %u num, %d action\n", id, num, action);
    //switch action
    switch(action){
        case 0: a = UP; break;
        case 1: a = RIGHT; break;
        case 2: a = DOWN; break;
        case 3: a = LEFT; break;
        case 4: a = PBOMB; break;
        case 5: a = QUIT;  break; 
    }
    //setgrid
    perform_action(board, pos, a, bomblist, id);//capter que le client a tenter de quitter aussi et gerer le cas

}

message_partie_client* perform_action_req(board* b, pos* p, ACTION a,bomblist *list, info_joueur *info_joueur, unsigned int num) {
    int xd = 0;
    int yd = 0;
    bool possible = false;

    message_partie_client *msg = malloc(sizeof(message_partie_client));
    if(!msg){
        perror("malloc perform_action_req");
        exit(-1);
    }

    switch (a) {
        case LEFT:
            xd = -2; yd = 0; break; //footsteps of size 2 for more fluidity !!!        
        case RIGHT:
            xd = 2; yd = 0; break;
        case UP:
            xd = 0; yd = -1; break;
        case DOWN:
            xd = 0; yd = 1; break;
        case PBOMB : 
         move_req(msg, info_joueur, a, num); return msg;
        case QUIT:
         move_req(msg, info_joueur, a, num); return msg;
        default: free(msg); return NULL; 
    }
    p->x += xd; p->y += yd;
    p->x = (p->x + b->w)%b->w;

    if(p->x == b->w-2){
        p->x = b->w-1;
    }
    else if(p->x == 1){
        p->x = 0;
    }

    p->y = (p->y + b->h)%b->h;
    if(get_grid(b,p->x,p->y) == EMPTY || get_grid(b,p->x,p->y) == EXPLODE){
        possible=true;
        move_req(msg, info_joueur, a, num);
    }
    if(!possible){
        free(msg); return NULL;
    }
    return msg;
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