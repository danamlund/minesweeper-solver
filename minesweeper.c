/* Dan Amlund Thomsen
   20 Jan. 2012
   dan@danamlund.dk
   www.danamlund.dk */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#include "minesweeper_lib.h"

static bool verbose = false;

struct board* generate_board(int size_x, int size_y, int mines) {
  struct neighbor_it *it;
  struct board *b;

  srand((unsigned int) time(NULL));

  b = new_board(size_x, size_y);

  int x, y, i = min(mines, size_x*size_y);
  while (i > 0) {
    x = rand() % size_x;
    y = rand() % size_y;
    if (TILE_MINE != b->adj_mines[x][y]) {
      i--;
      b->adj_mines[x][y] = TILE_MINE;
      for(it = new_neighbor_it(b, x, y); next_neighbor_it(it);) {
        if (b->adj_mines[it->x][it->y] >= 0)
          b->adj_mines[it->x][it->y]++;
      }
    }
  }
  return b;
}

char process_input(struct board* solution, struct board* input) {
  char output = OUT_NOT_CHANGED;
  int x, y;

  for (y = 0; y < input->size_y; y++) {
    for (x = 0; x < input->size_x; x++) {
      if (input->adj_mines[x][y] == TILE_NOT_MINE) {
        if (solution->adj_mines[x][y] == TILE_MINE) {
          input->adj_mines[x][y] = TILE_EXPLOSION;
          output = OUT_INVALID;
        } else {
          input->adj_mines[x][y] = solution->adj_mines[x][y];
          if (output != OUT_INVALID)
            output = OUT_CHANGED;
        }
      }
    }
  }

  bool again = true;
  struct neighbor_it *it;
  while (again) {
    again = false;
    for (y = 0; y < input->size_y; y++) {
      for (x = 0; x < input->size_x; x++) {
        if (input->adj_mines[x][y] == 0) {
          for(it = new_neighbor_it(input, x, y); next_neighbor_it(it);) {
            if (input->adj_mines[it->x][it->y] == TILE_UNKNOWN) {
              input->adj_mines[it->x][it->y] = 
                solution->adj_mines[it->x][it->y];
              again = true;
            }
          }
          free_neighbor_it(it);
        }
      }
    }
  }

  return output;
}

void start_new_game(char *name, int x, int y, int mines) {
  struct game *game = new_game();;

  game->x = x;
  game->y = y;
  game->mines = mines;
  game->seed = (int) time(NULL);

  game->solution = generate_board(x, y, mines);
  game->input = new_board(x, y);

  int xx, yy;
  for (yy = 0; yy < y; yy++) {
    for (xx = 0; xx < x; xx++) {
      game->input->adj_mines[xx][yy] = TILE_UNKNOWN;
    }
  }
  
  FILE *f;
  if (NULL == name || 0 == strcmp(name, "-")) {
    f = stdout;
  } else {
    f = fopen(name, "w");
  }
  save_game(game, f);
  fclose(f);
}

void process_game(char *file) {
  struct game *game;
  FILE *f;

  if (NULL == file || 0 == strcmp(file, "-")) {
    f = stdin;
  } else {
    f = fopen(file, "r");
  }
  game = load_game(f);
  fclose(f);

  if (NULL == game) {
    fprintf(stderr, "minesweeper: invalid game.\n");
    exit(1);
  }
  
  int output;
  output = process_input(game->solution, game->input);

  if (NULL == file || 0 == strcmp(file, "-")) {
    f = stdout;
  } else {
    f = fopen(file, "w");
  }
  save_game(game, f);
  fclose(f);

  switch (output) {
  case OUT_INVALID: 
    if (verbose) fprintf(stderr, "You hit a mine!\n"); 
    exit(0);
  case OUT_NOT_CHANGED: 
    if (verbose) fprintf(stderr, "No moves made.\n"); 
    exit(0);
  default: 
    if (verbose) fprintf(stderr, "Make your next move.\n");
  }
}

void print_help() {
  printf("Usage: minesweeper [OPTION]... [FILE]\n"
         "Start a new minesweeper game to FILE or standard output\n"
         "(see -n), or process a turn in an existing minesweeper\n"
         "game from FILE or standard input.\n"
         "\n"
         "  -n         Start a new game.\n"
         "  -b INT     Start a new game using a predefined board:\n"
         "               0: 8x8 grid with 10 mines\n"
         "               1: 16x16 grid with 40 mines\n"
         "               2: 30x16 grid with 99 mines\n"
         "               3: 78x21 grid with 450 mines\n"
         "  -x INT     Set the width of the grid.\n"
         "  -y INT     Set the height of the grid.\n"
         "  -m INT     Set the number of mines.\n"
         "  -h, --help Show this help menu.\n"
         "  -v         More output.\n"
         "\n"
         "The FILE contains information about the game and provide an\n"
         "interface to the game. Only modify the file by overwriting\n"
         "characters in the first grid, the reader is sensitive(bad).\n"
         "\n"
         "Legend of the symbols in the grid:\n"
         "  .      A unknown tile\n"
         "  space  A tile with no bombs surrounding it\n"
         "  1-8    A tile surrounded by 1-8 mines\n"
         "  !      A tile designated by the user as a mine\n"
         "  ?      A tile designated by the user as not a mine,\n"
         "           running minesweeper processed these\n"
         "  *      A exploded mine. If you see any of these you\n"
         "           have lost\n"
         "\n"
         "Example: Playing a game of minesweeper\n"
         "  Start a new game called using file 'foo':\n"
         "    run 'minesweeper -n -b 1 foo.txt'\n"
         "  Play minesweeper:\n"
         "    1) Edit file foo.txt\n"
         "    2) Replace .'s with ?'s or !'s to mark tiles that are \n"
         "       not mines and mines, respectively\n"
         "    3) Process your input by running 'minesweeper foo.txt'\n"
         "    4) Go to step 1)\n"
         );
  
}

int game(int argc, char** args) {
  char *file = NULL;
  bool newgame = false;
  int x = 0, y = 0, mines = 0, b = 0;

  int i = 1;
  while (i < argc) {
    if (0 == strcmp("--help", args[i])) {
      print_help();
      exit(0);
    }
    if (args[i][0] == '-') {
      switch (args[i][1]) {
      case 'h': print_help(); exit(0);
      case 'n': newgame = true; break;
      case 'x': x = atoi(args[i+1]); i++; break;
      case 'y': y = atoi(args[i+1]); i++; break;
      case 'm': mines = atoi(args[i+1]); i++; break;
      case 'b': b = atoi(args[i+1]); i++; break;
      case 'v': verbose = true; break;
      default: fprintf(stderr, "Unknown argument '%s'\n", args[i]); exit(1);
      }
    } else {
      file = args[i];
    }
    i++;
  }

  if (newgame) {
    int gx, gy, gmines;
    switch (b) {
    case 0: gx = 8, gy = 8, gmines = 10; break;
    case 1: gx = 16, gy = 16, gmines = 40; break;
    case 2: gx = 30, gy = 16, gmines = 99; break;
    case 3: gx = 78, gy = 21, gmines = 450; break;
    default: gx = 1, gy = 1, gmines = 1;
    }

    if (x) gx = x;
    if (y) gy = y;
    if (mines) gmines = mines;

    start_new_game(file, gx, gy, gmines);
  } else {
    process_game(file);
  }

  return 0;
}

int main(int argc, char** args) {
  return game(argc, args);
}
