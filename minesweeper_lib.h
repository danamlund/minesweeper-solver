#ifndef MINESWEEPER_LIB_H_
#define MINESWEEPER_LIB_H_

#include <stdio.h>
#include <stdlib.h>

#define min(X, Y)  ((X) < (Y) ? (X) : (Y))
#define max(X, Y)  ((X) > (Y) ? (X) : (Y))

#define TILE_UNKNOWN -1
#define TILE_MINE -2
#define TILE_NOT_MINE -3
#define TILE_EXPLOSION -4

#define OUT_CHANGED 0
#define OUT_NOT_CHANGED 1
#define OUT_INVALID -1

#define true 1
#define false 0

typedef char bool;

struct board {
  int** adj_mines;
  int size_x, size_y;
};

struct neighbor_it {
  struct board* board;
  int pos;
  int cx, cy, x, y;
};

struct mystring {
  char* str;
  int i;
};

struct game {
  int x, y, mines;
  struct board *input;
  int seed;
  struct board *solution;
};


struct neighbor_it* new_neighbor_it(struct board *board, int x, int y);
void free_neighbor_it(struct neighbor_it *it);
bool next_neighbor_it(struct neighbor_it *it);

struct board* new_board(int size_x, int size_y);
void free_board(struct board* board);
struct board* copy_board(struct board* board);

struct board* read_board(FILE* f, int sx, int sy);
void write_board(struct board* board, FILE *f);
void print_board(struct board* board);
void save_board(struct board *board, char *filename);

void print_out(char out);

struct game* new_game();
void free_game(struct game *game);
void encrypt_board(struct board *b, int seed);
void decrypt_board(struct board *b, int seed);
struct game* load_game(FILE *f);
void save_game(struct game *game, FILE *f);

bool file_exists(char *file);

#endif /* !MINESWEEPER_LIB_H_ */
