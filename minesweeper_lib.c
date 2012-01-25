#include <stdio.h>
#include <stdlib.h>

#include "minesweeper_lib.h"

struct neighbor_it* new_neighbor_it(struct board *board, int x, int y) {
  struct neighbor_it *it;

  it = (struct neighbor_it*) malloc(sizeof(struct neighbor_it));
  it->board = board;
  it->cx = x;
  it->cy = y;
  it->x = it->y = -1;
  it->pos = -1;
  return it;
}
void free_neighbor_it(struct neighbor_it *it) {
  free(it);
}
bool next_neighbor_it(struct neighbor_it *it) {
  bool found_valid = false;
  /* pos =
     012
     7x3
     654 */
  while (!found_valid) {
    it->pos++;
    if (it->pos > 7)
      return false;

    switch (it->pos) {
    case 0: it->x = it->cx - 1; it->y = it->cy - 1; break;
    case 1: it->x = it->cx + 0; it->y = it->cy - 1; break;
    case 2: it->x = it->cx + 1; it->y = it->cy - 1; break;
    case 3: it->x = it->cx + 1; it->y = it->cy + 0; break;
    case 4: it->x = it->cx + 1; it->y = it->cy + 1; break;
    case 5: it->x = it->cx + 0; it->y = it->cy + 1; break;
    case 6: it->x = it->cx - 1; it->y = it->cy + 1; break;
    case 7: it->x = it->cx - 1; it->y = it->cy + 0; break;
    }
    found_valid = ! ( it->x < 0 || it->x >= it->board->size_x ||
                      it->y < 0 || it->y >= it->board->size_y );

  }
  return true;
}

struct board* new_board(int size_x, int size_y) {
  struct board *board;

  board = (struct board*) malloc(sizeof(struct board));
  board->size_x = size_x;
  board->size_y = size_y;
  int x,y;
  board->adj_mines = (int**) malloc(sizeof(int) * size_x);
  for (x = 0; x < size_x; x++) {
    board->adj_mines[x] = (int*) malloc(sizeof(int) * size_y);
    for (y = 0; y < size_y; y++) {
      board->adj_mines[x][y] = 0;
    }
  }
  return board;
}

void free_board(struct board* board) {
  int x;
  for (x = 0; x < board->size_x; x++) {
    free(board->adj_mines[x]);
  }
  free(board);
}

struct board* copy_board(struct board* board) {
  struct board* copy = new_board(board->size_x, board->size_y);
  int x, y;
  for (x = 0; x < board->size_x; x++) {
    for (y = 0; y < board->size_y; y++) {
      copy->adj_mines[x][y] = board->adj_mines[x][y];
    }
  }
  return copy;
}

struct board* read_board(FILE* f, int sx, int sy) {
  int x = -1, y = -1, c;

  struct board* board = new_board(sx, sy);
  x = y = 0;
  while (x <= sx && y <= sy && EOF != (c = fgetc(f))) {
    if (c == '\n') {
      x = 0;
      y++;
    } else {
      if (c >= '0' && c <= '8')
        board->adj_mines[x][y] = c - '0';
      else switch (c) {
        case ' ': board->adj_mines[x][y] = 0; break;
        case '!': board->adj_mines[x][y] = TILE_MINE; break;
        case '?': board->adj_mines[x][y] = TILE_NOT_MINE; break;
        case '*': board->adj_mines[x][y] = TILE_EXPLOSION; break;
        default: board->adj_mines[x][y] = TILE_UNKNOWN;
        }
      x++;
    }
  }
  return board;
}

void write_board(struct board* board, FILE *f) {
  int x, y;
  for (y = 0; y < board->size_y; y++) {
    for (x = 0; x < board->size_x; x++) {
      switch (board->adj_mines[x][y]) {
      case TILE_UNKNOWN: fprintf(f, "."); break;
      case TILE_MINE: fprintf(f, "!"); break;
      case TILE_NOT_MINE: fprintf(f, "?"); break;
      case TILE_EXPLOSION: fprintf(f, "*"); break;
      case 0: fprintf(f, " "); break;
      default: fprintf(f, "%c", '0' + board->adj_mines[x][y]);
      }
    }
    fprintf(f, "\n");
  }
}

void print_board(struct board* board) {
  write_board(board, stdout);
}

void save_board(struct board *board, char *filename) {
  FILE *f = fopen(filename, "w");
  if (f == NULL) {
    printf("Couldn't open file '%s'\n", filename);
    exit(1);
  }
  write_board(board, f);
  fclose(f);
}

void print_out(char out) {
  printf("output: %s\n", (out == OUT_INVALID ? "INVALID" :
                          (out == OUT_NOT_CHANGED ? "NOT_CHANGED" :
                           "CHANGED")));
}

struct game* new_game() {
  struct game *game;
  game = (struct game*) malloc(sizeof(struct game));
  game->input = game->solution = NULL;
  game->x = game->y = game->mines = game->seed -1;
  return game;
}
void free_game(struct game *game) {
  free(game);
}

void encrypt_board(struct board *b, int seed) {
  int x, y;

  srand(seed);
  for (y = 0; y < b->size_y; y++) {
    for (x = 0; x < b->size_x; x++) {
      if (b->adj_mines[x][y] < 0 && b->adj_mines[x][y] != TILE_MINE) {
        printf("Error encrypting board.\n");
        exit(1);
      }
      b->adj_mines[x][y] = ((b->adj_mines[x][y] + 4 
                             + (rand() % 13)) % 13) - 4;
    }
  }
}

void decrypt_board(struct board *b, int seed) {
  int x, y;

  srand(seed);
  for (y = 0; y < b->size_y; y++) {
    for (x = 0; x < b->size_x; x++) {
      b->adj_mines[x][y] = ((13 +
                             (b->adj_mines[x][y] + 4 
                              - (rand() % 13))) % 13) - 4;
      if (b->adj_mines[x][y] < 0 && b->adj_mines[x][y] != TILE_MINE) {
        printf("Error descrypting board.\n");
        exit(1);
      }
    }
  }
}

struct game* load_game(FILE *f) {
  char *name = (char*) malloc(sizeof(char)*100);
  int x, y, mines, seed;
  struct board *input, *solution;

  if (3 != fscanf(f, "Minesweeper %dx%d grid with %d mines",
                  &x, &y, &mines)) {
    free(name);
    return NULL;
  }
  fgetc(f);
  input = read_board(f, x, y);

  if (1 != fscanf(f, "Solution encrypted with seed %d", &seed)) {
    free(name);
    free_board(input);
    return NULL;
  }
  fgetc(f);
  solution = read_board(f, x, y);
  decrypt_board(solution, seed);

  struct game *game = new_game();
  game->x = x;
  game->y = y;
  game->mines = mines;
  game->input = input;
  game->seed = seed;
  game->solution = solution;

  return game;
}

void save_game(struct game *game, FILE *f) {
  fprintf(f, "Minesweeper %dx%d grid with %d mines\n",
          game->x, game->y, game->mines);
  write_board(game->input, f);
  fprintf(f, "\n"
          "Solution encrypted with seed %d\n", game->seed);
  struct board *solution = copy_board(game->solution);
  encrypt_board(solution, game->seed);
  write_board(solution, f);
  free_board(solution);
}

bool file_exists(char *file) {
  FILE *f = fopen(file, "r");
  if (f != NULL) {
    fclose(f);
    return true;
  }
  return false;
}
