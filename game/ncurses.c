// Build with -lncurses option

#include "game.h"
#include "myncurses.h"
#include "../client/auxiliaire.h"
#include <ncurses.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

//list

bomblist *create_list (int capacity){
    bomblist* res = malloc(sizeof(bomblist));
    if(!res) goto error;

    res->capacity = capacity;
    res->length = 0;
    res->bcontainer = malloc(sizeof(bomb*)*capacity);

    if(!res->bcontainer) goto error;

    return res;
    error:
    exit(1);
}

void add_list (bomblist *list, bomb* data){

    if(list->capacity == list->length){
        list->capacity = list->capacity*2;
        list->bcontainer = realloc(list->bcontainer, sizeof(bomb)*list->capacity);
        if(!list->bcontainer){
            goto error;
        
        }
    }

    list->bcontainer[list->length] = data; 
    list->length++;

    return;

    error:
    exit(1);
}

//remove element
int remove_bomb(bomblist *list, bomb *data){ 

    int i = 0;
    bool deleted = false;

    for(i; i<list->length; i++){
        if(list->bcontainer[i] == data){
            free_bomb(list->bcontainer[i]);
            deleted = true;
            break;
        }

    }

    if(deleted){
        memmove(list->bcontainer+i,list->bcontainer+i+1,(list->length-i-1)*sizeof(bomb));
        list->length--;
    }
    else{
        return -1;
    }

    return 0;
}


//vide la liste

void empty_list(bomblist *list){

    for(int i=0; i<list->length; i++){
       free_bomb(list->bcontainer[i]);
    }

    free(list->bcontainer);
    list->bcontainer = NULL;
    free(list);
    list=NULL;
}

//bomb


bomb* init_bomb(int x, int y){ //stocker bomb dans une liste de bombe
    bomb *res= malloc(sizeof(bomb));
    res->bpos = malloc(sizeof(pos));
    res->timeleft = 3*1000000; //3 sec
    res->bpos->x = x;
    res->bpos->y = y;
    return res;
}

void free_bomb(bomb * b){
    free(b->bpos);
    free(b); 
}


bool bomb_explode(bomb* bomb, board *board, bomblist *list, OBJECT obj, bool death[4]){
    pos *p = bomb->bpos;
    bool res= false;
    int width = board->w;
    int height = board->h;
    //x+2 reminder footstep size 2
  
    if(p->x+2 < width && p->x+2 > 0){
    if(get_grid(board,p->x+2,p->y) != WALL){
        //delete
        if(get_grid(board,p->x+2,p->y) <= CHARACTER4){ 
            
            death[get_grid(board,p->x+2,p->y)] = true;
            res = true;

        }

        if(get_grid(board,p->x+2,p->y) != BWALL && get_grid(board,p->x+2,p->y) != WALL ){
            //continu casser niveau 2
            if(get_grid(board,p->x+4,p->y) != WALL){
                if(get_grid(board,p->x+4,p->y) <= CHARACTER4){ 
            
            death[get_grid(board,p->x+4,p->y)] = true;
            res = true;

        }
                if(!(res && obj == EXPLODE)){
                set_grid(board,p->x+4,p->y, obj);//niveau 2
                }
            }
        }
        if(!(res && obj == EXPLODE)){
        set_grid(board,p->x+2,p->y, obj);//niveau 1
        }
    }
    }

    //y+1
    if(p->y+1 < height && p->y+1 >0){
    if(get_grid(board,p->x,p->y+1) != WALL){
        //delete
        if(get_grid(board,p->x,p->y+1) <= CHARACTER4){ 
            
            death[get_grid(board,p->x,p->y+1)] = true;
            res = true;

        }
        if(get_grid(board,p->x,p->y+1) != BWALL && get_grid(board,p->x,p->y+1) != WALL){
            //continu casser niveau 2
            if(get_grid(board,p->x,p->y+2) != WALL){
                if(get_grid(board,p->x,p->y+2) <= CHARACTER4){ 
            
                     death[get_grid(board,p->x,p->y+2)] = true;
                     res = true;

        }
                if(!(res && obj == EXPLODE)){
                set_grid(board,p->x,p->y+2, obj);//niveau 2
                }
            }
        }
        if(!(res && obj == EXPLODE)){
        set_grid(board,p->x,p->y+1, obj);//niveau 1
        }
    }
    }

    if(p->x-2 < width && p->x-2 > 0){
    //x-2 reminder footstep size 2
    if(get_grid(board,p->x-2,p->y) != WALL){
        //delete
        if(get_grid(board,p->x-2,p->y) <= CHARACTER4){ 
            
            death[get_grid(board,p->x-2,p->y)] = true;
            res = true;

        }
        if(get_grid(board,p->x-2,p->y) != BWALL && get_grid(board,p->x-2,p->y) != WALL){
            //continu casser niveau 2
            if(get_grid(board,p->x-4,p->y) != WALL){
                if(get_grid(board,p->x-4,p->y) <= CHARACTER4){ 
            
            death[get_grid(board,p->x-4,p->y)] = true;
            res = true;

        }
                if(!(res && obj == EXPLODE)){
                set_grid(board,p->x-4,p->y, obj);//niveau 2
                }
            }
        }
        if(!(res && obj == EXPLODE)){
        set_grid(board,p->x-2,p->y, obj);//niveau 1
        }
    }
    }

    if(p->y-1 < height && p->y-1 > 0){
    //y-1
    if(get_grid(board,p->x,p->y-1) != WALL){
        //delete
        if(get_grid(board,p->x,p->y-1) <= CHARACTER4){ 
            
            death[get_grid(board,p->x,p->y-1)] = true;
            res = true;

        }
        if(get_grid(board,p->x,p->y-1) != BWALL && get_grid(board,p->x,p->y-1) != WALL){
            //continu casser niveau 2
            if(get_grid(board,p->x,p->y-2) != WALL){
                if(get_grid(board,p->x,p->y-2) <= CHARACTER4){ 
            
                    death[get_grid(board,p->x,p->y-2)] = true;
                    res = true;

        }
                if(!(res && obj == EXPLODE)){
                set_grid(board,p->x,p->y-2, obj);//niveau 2
                }
            }
        }
        if(!(res && obj == EXPLODE)){
        set_grid(board,p->x,p->y-1, obj);//niveau 1
        }
    }

    }


    if(p->x+2 < width && p->x+2 > 0 && p->y-1 < height && p->y-1 > 0){
    //x+2 y-1 reminder footstep size 2
    if(get_grid(board,p->x+2,p->y-1) != WALL){
        //delete
        if(get_grid(board,p->x+2,p->y-1) <= CHARACTER4){ 
            
            death[get_grid(board,p->x+2,p->y-1)] = true;
            res = true;

        }
        if(!(res && obj == EXPLODE)){
        set_grid(board,p->x+2,p->y-1, obj);//niveau 1
        }
    }
    }

    if(p->x+2 < width && p->x+2 > 0 && p->y+1 < height && p->y+1 > 0){
    //x+2 y+1
    if(get_grid(board,p->x+2,p->y+1) != WALL){
        //delete
        if(get_grid(board,p->x+2,p->y+1) <= CHARACTER4){ 
            
            death[get_grid(board,p->x+2,p->y+1)] = true;
            res = true;

        }
        if(!(res && obj == EXPLODE)){
        set_grid(board,p->x+2,p->y+1, obj);//niveau 1
        }
    }
    }

    if(p->x-2 < width && p->x-2 > 0 && p->y-1 < height && p->y-1 > 0){
    //x-2 y-1
    if(get_grid(board,p->x-2,p->y-1) != WALL){
        //delete
        if(get_grid(board,p->x-2,p->y-1) <= CHARACTER4){ 
            
            death[get_grid(board,p->x-2,p->y-1)] = true;
            res = true;

        }
            if(!(res && obj == EXPLODE)){
        set_grid(board,p->x-2,p->y-1, obj);//niveau 1
            }
    }

    }
    //x-2 y+1
    if(p->x-2 < width && p->x-2 > 0 && p->y+1 < height && p->y+1 > 0){
    if(get_grid(board,p->x-2,p->y+1) != WALL){
        //delete
        if(get_grid(board,p->x-2,p->y+1) <= CHARACTER4){ 
            
            death[get_grid(board,p->x-2,p->y+1)] = true;
            res = true;

        }
        if(!(res && obj == EXPLODE)){
        set_grid(board,p->x-2,p->y+1, obj);//niveau 1
        }
    }
    }

if(obj == EMPTY){
    set_grid(board, bomb->bpos->x, bomb->bpos->y, EMPTY); //retire la bombe plateau
    remove_bomb(list,bomb); //retire bombe de la liste
}
    return res;

} 

bool maj_bomb(bomblist *list, int timepassed, board *board, bool death[4]){
    bomblist *tmp = list;
    bool res = false;
    for(int i=0 ; i<list->length; i++){
        if(tmp->bcontainer[i] != NULL){
        tmp->bcontainer[i]->timeleft = tmp->bcontainer[i]->timeleft - timepassed;
        if(tmp->bcontainer[i]->timeleft <= 0){
            res = bomb_explode(tmp->bcontainer[i], board, list, EMPTY, death);
          }
        else if(tmp->bcontainer[i]->timeleft <= 1000000){
            bomb_explode(tmp->bcontainer[i], board, list, EXPLODE, death);
          }
        }
    }
    return res;
}; 


//grid 


void maze1(board* board){
    srand(time(NULL));
    int r;
    for(int i=0; i<board->h; i++){
        for(int j=0; j<board->w; j++){
           set_grid(board,j,i,EMPTY); 
        }
    }

    for(int i=1; i<board->h-1; i+=2){ //==%2 impair
        for(int j=4; j<board->w-1; j+=4){ //%4 mod 4
            set_grid(board,j,i,WALL);
        }
    }

    for (int i=0; i<board->h; i++){
        for(int j=0; j<board->w; j++){
            if(i%2 == 0 && j%2 == 0){
                r = rand()%3;
                if(r == 0){
                    set_grid(board,j,i,BWALL);
                }
            }
        }

    }
}

void setup_board(board* board) {
    int lines=28; 
    int columns=53;
    board->h = lines - 2 - 1; // 2 rows reserved for border, 1 row for chat
    board->w = columns - 2; // 2 columns reserved for border
    board->grid = calloc((board->w)*(board->h),sizeof(char));
    maze1(board);

    getmaxyx(stdscr,lines,columns);
}

void free_board(board* board) {
    free(board->grid);
}

int get_grid(board* b, int x, int y) {
    return b->grid[y*b->w + x];
}

void set_grid(board* b, int x, int y, int v) {
    b->grid[y*b->w + x] = v;
}

void setbomb(board* b, pos* p, bomblist *list){
    bomb *bomb = init_bomb(p->x, p->y);
    add_list(list,bomb);//peut ne pas fonctionner
    set_grid(b, p->x, p->y, BOMB);
}

//game

void refresh_game(board* b, line* l) {
    // Update grid
    int x,y;
    for (y = 0; y < b->h; y++) {
        for (x = 0; x < b->w; x++) {
            char c;
            switch (get_grid(b,x,y)) {
                case EMPTY:
                    c = ' ';
                    break;
                case CHARACTER:
                    c = 'O';
                    break;
                case CHARACTER2:
                    c = 'O';
                    break;
                case CHARACTER3:
                    c = 'O';
                    break; 
                case CHARACTER4:
                    c = 'O';
                    break;                  
                case WALL:
                    c= '-';
                    break;
                case BWALL:
                    c= '=';
                    break;  
                case BOMB:
                    c='o'; 
                    break; 
                case EXPLODE:
                    c= '.'; break;                 
                default:
                    c = '?';
                    break;
            }
            if(c == 'O'){
                attron(COLOR_PAIR(2));
            }
            else if(c == '.'){
                attron(COLOR_PAIR(3));
            }
            mvaddch(y+1,x+1,c);
            if(c == 'O'){
                attroff(COLOR_PAIR(2));
            }
            else if(c == '.'){
                attroff(COLOR_PAIR(3));
            }
        }
    }
    for (x = 0; x < b->w+2; x++) {
        mvaddch(0, x, '-');
        mvaddch(b->h+1, x, '-');
    }
    for (y = 0; y < b->h+2; y++) {
        mvaddch(y, 0, '|');
        mvaddch(y, b->w+1, '|');
    }
    if(l != NULL){
    // Update chat text
    attron(COLOR_PAIR(1)); // Enable custom color 1
    attron(A_BOLD); // Enable bold
    for (x = 0; x < b->w+2; x++) {
        if (x >= TEXT_SIZE || x >= l->cursor)
            mvaddch(b->h+2, x, ' ');
        else
            mvaddch(b->h+2, x, l->data[x]);
    }
    attroff(A_BOLD); // Disable bold
    attroff(COLOR_PAIR(1)); // Disable custom color 1
    }
    refresh(); // Apply the changes to the terminal
}

ACTION control(line* l, int sock_tcp, info_joueur * info_joueur) { //Reecrire cette fonc mais de maniere à faire une requete à envoyer au serveur
    int c;
    int prev_c = ERR;
    // We consume all similar consecutive key presses
    while ((c = getch()) != ERR) { // getch returns the first key press in the queue
        if (prev_c != ERR && prev_c != c) {
            ungetch(c); // put 'c' back in the queue
            break;
        }
        prev_c = c;
    }
    char buf[100];
    memset(buf, 0, 100);
    ACTION a = NONE;
    switch (prev_c) {
        case ERR: break;
        case KEY_LEFT:
            a = LEFT; break;
        case KEY_RIGHT:
            a = RIGHT; break;
        case KEY_UP:
            a = UP; break;
        case KEY_DOWN:
            a = DOWN; break;

        case 10:
            a = PBOMB; break;
        case '~':
            a = QUIT; break;

        
        case KEY_BACKSPACE:
            if (l->cursor > 0) (l->cursor)--;
            break;
        case '%' : // envoyer un message
            if (l->clean == 0){
                memcpy(buf, l->data, l->cursor);
                send_message(sock_tcp,info_joueur, buf, 7); // à changer pour le mode en équipe
                l->cursor = 0;
            }
            break;
        default:
            if (l->clean) {
                l->cursor = 0;
                l->clean = 0;
            }
            if (prev_c >= ' ' && prev_c <= '~' && l->cursor < TEXT_SIZE)
                l->data[(l->cursor)++] = prev_c;
            break;
    }
    return a;
}


bool perform_action(board* b, pos* p, ACTION a,bomblist *list, int character) {//maj la pos et la grid
    int xd = 0;
    int yd = 0;
    int prevx = p->x;
    int prevy = p->y;
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
            setbomb(b,p,list); return false;
        case QUIT:
            return true;
        default: return false; 
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
    set_grid(b,p->x,p->y,character);
    }
    else { //si mur pos change pas
        p->x = prevx;
        p->y = prevy;
    }
    if(!(prevx == p->x && prevy == p->y) && get_grid(b,prevx,prevy) != BOMB){ //pour pas que le "chemin" du perso s'affiche, si la pos a changé efface la case
        set_grid(b,prevx,prevy,EMPTY);
    }
    return false;
}

void print_message (int id, char * s, line * l){
    l->cursor = 0;
    char text [TEXT_SIZE];
    sprintf(text, "joueur %d : %s",id, s);
    for (int i=0; i<strlen(text);i++){
        l->data[(l->cursor)++] = text[i];
    }
    l->clean = 1;
}
