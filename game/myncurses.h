#include <stdbool.h>
#ifndef NCURSES_H
#define NCURSES_H

#define TEXT_SIZE 255
#define GRID_SIZE 1275

typedef enum ACTION { NONE, UP, DOWN, LEFT, RIGHT, PBOMB, QUIT } ACTION;
typedef enum OBJECT { EMPTY, BOMB, WALL, BWALL, CHARACTER, CHARACTER2, CHARACTER3, CHARACTER4} OBJECT; //breakable wall = wall - //character1 - 4 not implemented yet

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

#endif