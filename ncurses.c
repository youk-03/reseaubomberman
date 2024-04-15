// Build with -lncurses option

#include <ncurses.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define TEXT_SIZE 255

typedef enum ACTION { NONE, UP, DOWN, LEFT, RIGHT, PBOMB, QUIT } ACTION;
typedef enum OBJECT { EMPTY, CHARACTER, BOMB, WALL, BWALL} OBJECT; //breakable wall = wall - 

typedef struct board {
    char* grid;
    int w;
    int h;
} board;

typedef struct line {
    char data[TEXT_SIZE];
    int cursor;
} line;

typedef struct pos {
    int x;
    int y;
} pos;

typedef struct bomb {
   pos *bpos;
   int timeleft; 
} bomb;

typedef struct bomblist{
    struct bomb** bcontainer;
    int capacity;
    int length;
} bomblist;

bomblist *create_list (int capacity);
void add_list (bomblist *list, bomb* data);
int remove_bomb(bomblist *list, bomb *data);
//void free_element(bomblist *el);
void empty_list(bomblist *list);
bomb* init_bomb(int x, int y);
void free_bomb(bomb * b);
bool bomb_explode(bomb* bomb, board *board, bomblist *list);
bool maj_bomb(bomblist *list, int timepassed, board *board);
void set_grid(board* b, int x, int y, int v);
void maze1(board* board);
void setup_board(board* board);
void free_board(board* board);
int get_grid(board* b, int x, int y);
void set_grid(board* b, int x, int y, int v);
void setbomb(board* b, pos* p, bomblist *list);
void refresh_game(board* b, line* l);
ACTION control(line* l);
bool perform_action(board* b, pos* p, ACTION a,bomblist *list);

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

bool bomb_explode(bomb* bomb, board *board, bomblist *list){//true si character meurt
    pos *p = bomb->bpos;
    bool res= false;
    //x+1
    if(get_grid(board,p->x+1,p->y) != WALL){
        //delete
        if(get_grid(board,p->x+1,p->y) == CHARACTER) res = true;
        if(get_grid(board,p->x+1,p->y) != BWALL){
            //continu casser niveau 2
            if(get_grid(board,p->x+2,p->y) != WALL){
                if(get_grid(board,p->x+2,p->y) == CHARACTER) res = true;
                set_grid(board,p->x+2,p->y, EMPTY);//niveau 2
            }
        }
        set_grid(board,p->x+1,p->y, EMPTY);//niveau 1
    }

    //y+1
    if(get_grid(board,p->x,p->y+1) != WALL){
        //delete
        if(get_grid(board,p->x,p->y+1) == CHARACTER) res = true;
        if(get_grid(board,p->x,p->y+1) != BWALL){
            //continu casser niveau 2
            if(get_grid(board,p->x,p->y+2) != WALL){
                if(get_grid(board,p->x,p->y+2) == CHARACTER) res = true;
                set_grid(board,p->x,p->y+2, EMPTY);//niveau 2
            }
        }
        set_grid(board,p->x,p->y+1, EMPTY);//niveau 1
    }

    //x-1
    if(get_grid(board,p->x-1,p->y) != WALL){
        //delete
        if(get_grid(board,p->x-1,p->y) == CHARACTER) res = true;
        if(get_grid(board,p->x-1,p->y) != BWALL){
            //continu casser niveau 2
            if(get_grid(board,p->x-2,p->y) != WALL){
                if(get_grid(board,p->x-2,p->y) == CHARACTER) res = true;
                set_grid(board,p->x-2,p->y, EMPTY);//niveau 2
            }
        }
        set_grid(board,p->x-1,p->y, EMPTY);//niveau 1
    }

    //y-1
    if(get_grid(board,p->x,p->y-1) != WALL){
        //delete
        if(get_grid(board,p->x,p->y-1) == CHARACTER) res = true;
        if(get_grid(board,p->x,p->y-1) != BWALL){
            //continu casser niveau 2
            if(get_grid(board,p->x,p->y-2) != WALL){
                if(get_grid(board,p->x,p->y-2) == CHARACTER) res = true;
                set_grid(board,p->x,p->y-2, EMPTY);//niveau 2
            }
        }
        set_grid(board,p->x,p->y+1, EMPTY);//niveau 1
    }

    //x+1 y-1
    if(get_grid(board,p->x+1,p->y-1) != WALL){
        //delete
        if(get_grid(board,p->x+1,p->y-1) == CHARACTER) res = true;
        set_grid(board,p->x+1,p->y-1, EMPTY);//niveau 1
    }
    //x+1 y+1
    if(get_grid(board,p->x+1,p->y+1) != WALL){
        //delete
        if(get_grid(board,p->x+1,p->y+1) == CHARACTER) res = true;
        set_grid(board,p->x+1,p->y+1, EMPTY);//niveau 1
    }
    //x-1 y-1
    if(get_grid(board,p->x-1,p->y-1) != WALL){
        //delete
        if(get_grid(board,p->x-1,p->y-1) == CHARACTER) res = true;
        set_grid(board,p->x-1,p->y-1, EMPTY);//niveau 1
    }
    //x-1 y+1
    if(get_grid(board,p->x-1,p->y+1) != WALL){
        //delete
        if(get_grid(board,p->x-1,p->y+1) == CHARACTER) res = true;
        set_grid(board,p->x-1,p->y+1, EMPTY);//niveau 1
    }

    set_grid(board, bomb->bpos->x, bomb->bpos->y, EMPTY); //retire la bombe plateau
    remove_bomb(list,bomb); //retire bombe de la liste
    return res;

} 

bool maj_bomb(bomblist *list, int timepassed, board *board){
    bomblist *tmp = list;
    bool res = false;
    for(int i=0 ; i<list->length; i++){
        if(tmp->bcontainer[i] != NULL){
        tmp->bcontainer[i]->timeleft = tmp->bcontainer[i]->timeleft - timepassed;
        if(tmp->bcontainer[i]->timeleft <= 0){
            res = bomb_explode(tmp->bcontainer[i], board, list);
          } 
        }
    }
    return res;
}; //appel bomb explode si une bombe doit exploser, deduit time passed du timeleft de chaque bomb


//grid 


void maze1(board* board){
    srand(time(NULL));
    int r;
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
    int lines; int columns;
    getmaxyx(stdscr,lines,columns);
    board->h = lines - 2 - 1; // 2 rows reserved for border, 1 row for chat
    board->w = columns - 2; // 2 columns reserved for border
    board->grid = calloc((board->w)*(board->h),sizeof(char));
    maze1(board);
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
                case WALL:
                    c= '-';
                    break;
                case BWALL:
                    c= '=';
                    break;  
                case BOMB:
                    c='o'; 
                    break;                  
                default:
                    c = '?';
                    break;
            }
            mvaddch(y+1,x+1,c);
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
    refresh(); // Apply the changes to the terminal
}

ACTION control(line* l) {
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
            if (l->cursor > 0) l->cursor--;
            break;
        default:
            if (prev_c >= ' ' && prev_c <= '~' && l->cursor < TEXT_SIZE)
                l->data[(l->cursor)++] = prev_c;
            break;
    }
    return a;
}


bool perform_action(board* b, pos* p, ACTION a,bomblist *list) {
    int xd = 0;
    int yd = 0;
    int prevx = p->x;
    int prevy = p->y;
    switch (a) {
        case LEFT:
            xd = -2; yd = 0; break;
        case RIGHT:
            xd = 2; yd = 0; break;
        case UP:
            xd = 0; yd = -1; break;
        case DOWN:
            xd = 0; yd = 1; break;
        case PBOMB : 
            setbomb(b,p,list); return false;/////////////////////////////////////////
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
    if(get_grid(b,p->x,p->y) == EMPTY){
    set_grid(b,p->x,p->y,CHARACTER);
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



int main()
{
    board* b = malloc(sizeof(board));;
    line* l = malloc(sizeof(line));
    l->cursor = 0;
    pos* p = malloc(sizeof(pos));
    p->x = 0; p->y = 0;

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

    setup_board(b);
    int tick = 30*1000;
    bomblist *list =create_list(10);

    while (true) {
        ACTION a = control(l);
        if (perform_action(b, p, a, list)) break;
        refresh_game(b,l);
        usleep(tick);
        if(maj_bomb(list,tick,b)) break;
        //3 sec plus tard bomb explode stocker la valeur en tant depuis que la bomb a été posé dans une struct bomb et une fois 3 secatteinte la faire exploser
        //je suis en train de faire de l'objet avec du c si c'est pas beau ça
    }

    printf("gameover");
    free_board(b);
    empty_list(list);

    curs_set(1); // Set the cursor to visible again
    endwin(); /* End curses mode */

    free(p); free(l); free(b);

    return 0;
    
}