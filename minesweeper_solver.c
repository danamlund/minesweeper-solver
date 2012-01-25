/* Dan Amlund Thomsen
   20 Jan. 2012
   dan@danamlund.dk
   www.danamlund.dk */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <limits.h>
#include <float.h>

/* #include <X11/Xlib.h> */
/* #include <X11/Xutil.h> */

#include "minesweeper_lib.h"

#define SCORE_MUL_NOT_MINES 0.3
#define SCORE_MUL_UNKNOWNS 0.1
#define SCORE_MUL_MINES 0.3
#define SCORE_MUL_CHANCE 2.5
#define SCORE_MUL_PROPAGATE 1.0

static int max_cluster_size = 15;
static bool verbose = false;
static bool fix_found_mines = false;

char propagate_tile(struct board *board, int x, int y) {
  struct neighbor_it *it;
  int nx, ny, mines, unknowns, board_mines, n;

  board_mines = board->adj_mines[x][y];
  unknowns = 0;
  mines = 0;
  
  for(it = new_neighbor_it(board, x, y); next_neighbor_it(it);) {
    n = board->adj_mines[it->x][it->y];
    if (n == TILE_UNKNOWN)
      unknowns++;
    if (n == TILE_MINE)
      mines++;
  }
  free_neighbor_it(it);

  if (mines > board_mines ||
      (unknowns == 0 && mines != board_mines)) {
    return OUT_INVALID;
  }

  bool have_changed_something = false;
  bool changed = true;
  while (changed) {
    changed = false;
    for(it = new_neighbor_it(board, x, y); next_neighbor_it(it);) {
      nx = it->x;
      ny = it->y;
      n = board->adj_mines[nx][ny];

      if (n == TILE_UNKNOWN) {
        if (board_mines - mines == unknowns) {
          board->adj_mines[nx][ny] = TILE_MINE;
          mines++;
          unknowns--;
          changed = true;
        }
        else if (board_mines == mines) {
          board->adj_mines[nx][ny] = TILE_NOT_MINE;
          unknowns--;
          changed = true;
        }
      }
    }
    free_neighbor_it(it);
    if (changed)
      have_changed_something = true;
  }
  return have_changed_something ? OUT_CHANGED : OUT_NOT_CHANGED;
}

char propagate_board(struct board *board) {
  int x, y;
  bool changed = true, have_changed_something = false;

  while (changed) {
    changed = false;
    for (y = 0; y < board->size_y; y++) {
      for (x = 0; x < board->size_x; x++) {
        if (board->adj_mines[x][y] > 0)
          switch (propagate_tile(board, x, y)) {
          case OUT_CHANGED: changed = true; break;
          case OUT_NOT_CHANGED: break;
          case OUT_INVALID: return OUT_INVALID;
          }
      }
    }
    if (changed)
      have_changed_something = true;
  }

  return have_changed_something ? OUT_CHANGED : OUT_NOT_CHANGED;
}


bool next_state(char *state, int unknowns_with_neighbors) {
  int i;
  for (i = unknowns_with_neighbors - 1; i >=0; i--) {
    if (state[i] == TILE_NOT_MINE) {
      state[i] = TILE_MINE;
      return true;
    } else {
      state[i] = TILE_NOT_MINE;
    }
  }
  
  return false;
}

bool is_valid_tile(struct board *b, int x, int y) {
  struct neighbor_it *it;
  int mines = 0, unknowns = 0;
  for(it = new_neighbor_it(b, x, y); next_neighbor_it(it);) {
    switch (b->adj_mines[it->x][it->y]) {
    case TILE_UNKNOWN:
      unknowns++;
      break;
    case TILE_MINE:
      mines++;
      break;
    }
  }
  free_neighbor_it(it);

  return ! (mines > b->adj_mines[x][y] ||
            (unknowns == 0 && mines != b->adj_mines[x][y]));
}

bool is_valid_state(char *state, int unknowns_with_neighbors, 
                    int **xys, struct board *b) {
  bool output = true;
  struct board *c = copy_board(b);
  int i, x, y;
  for (i = 0; i < unknowns_with_neighbors; i++) {
    x = xys[i][0];
    y = xys[i][1];
    c->adj_mines[x][y] = state[i];
  }

  struct board *valids = new_board(b->size_x, b->size_y);
  struct neighbor_it *it;
  for (i = 0; i < unknowns_with_neighbors; i++) {
    x = xys[i][0];
    y = xys[i][1];
    for(it = new_neighbor_it(c, x, y); next_neighbor_it(it);) {
      if (c->adj_mines[it->x][it->y] > 0 
          && valids->adj_mines[it->x][it->y] != 8) {
        valids->adj_mines[it->x][it->y] = 8;
        if (!is_valid_tile(c, it->x, it->y)) {
          output = false;
          break;
        }
      }
    }
    free_neighbor_it(it);
    if (!output)
      break;
  }
  free_board(c);
  free_board(valids);

  return output;
}

char constraint_satisfaction(struct board *b) {
  int **clusters = (int **) malloc(sizeof(int*)*b->size_x);
  int x, y;
  for (x = 0; x < b->size_x; x++) {
    clusters[x] = (int *) malloc(sizeof(int)*b->size_y);
    for (y = 0; y < b->size_y; y++) {
      clusters[x][y] = -1;
    }
  }

  struct neighbor_it *it;
  int cluster_id = 0;
  int cluster_cur;
  bool changed = true;
  while (changed) {
    changed = false;
    for (y = 0; y < b->size_y; y++) {
      for (x = 0; x < b->size_x; x++) {
        if (b->adj_mines[x][y] < 1)
          continue;
        cluster_cur = INT_MAX;
        for(it = new_neighbor_it(b, x, y); next_neighbor_it(it);) {
          if (clusters[it->x][it->y] != -1) {
            cluster_cur = min(cluster_cur, clusters[it->x][it->y]);
          }
        }
        free_neighbor_it(it);
        if (cluster_cur == INT_MAX) {
          cluster_cur = cluster_id;
          cluster_id++;
        }
        for(it = new_neighbor_it(b, x, y); next_neighbor_it(it);) {
          if (b->adj_mines[it->x][it->y] == TILE_UNKNOWN &&
              clusters[it->x][it->y] != cluster_cur) {
            clusters[it->x][it->y] = cluster_cur;
            changed = true;
          }
        }
        free_neighbor_it(it);
      }
    }
  }

  bool board_changed = false;
  while (true) {
    /* Find next cluster and count size of it */
    int cluster_size = 0;
    cluster_cur = -1;
    for (y = 0; y < b->size_y; y++) {
      for (x = 0; x < b->size_x; x++) {
        if (clusters[x][y] != -1 && cluster_cur == -1) {
          cluster_cur = clusters[x][y];
        }
        if (clusters[x][y] == -1 || clusters[x][y] != cluster_cur) {
          continue;
        }
        cluster_size++;
      }
    }
    if (cluster_cur == -1) {
      break;
    }

    /* Verify cluster is valid (neighbors more than 1 known tile) */
    struct board *verify = new_board(b->size_x, b->size_y);
    int neighbors = 0;
    for (y = 0; y < b->size_y; y++) {
      for (x = 0; x < b->size_x; x++) {
        if (clusters[x][y] != cluster_cur)
          continue;
        for(it = new_neighbor_it(b, x, y); next_neighbor_it(it);) {
          if (b->adj_mines[it->x][it->y] > 0 &&
              verify->adj_mines[it->x][it->y] != 8) {
            verify->adj_mines[it->x][it->y] = 8;
            neighbors++;
            if (neighbors >= 2) break;
          }
        }
        free_neighbor_it(it);
        if (neighbors >= 2) break;
      }
    }

    if (cluster_size > max_cluster_size || neighbors < 2) {
      if (cluster_size > max_cluster_size && verbose) {
        fprintf(stderr, "Skipping constraining because cluster is too big"
                " (%d > %d)\n", cluster_size, max_cluster_size);
      }
      for (y = 0; y < b->size_y; y++) {
        for (x = 0; x < b->size_x; x++) {
          if (clusters[x][y] == cluster_cur) {
            clusters[x][y] = -1;
          }
        }
      }
      continue;
    } else {
      if (verbose)
        fprintf(stderr, "Constraining cluster size %d.\n", cluster_size);
    }

    /* Initialize cluster variables */
    int **xys = (int**) malloc(sizeof(int)*cluster_size);
    char *common = (char*) malloc(sizeof(char)*cluster_size);
    char *state = (char*) malloc(sizeof(char)*cluster_size);
    int i;
    for (i = 0; i < cluster_size; i++) {
      xys[i] = (int*) malloc(sizeof(int)*2);
      xys[i][0] = xys[i][1] = -1;
      state[i] = TILE_NOT_MINE;
      common[i] = TILE_UNKNOWN;
    }

    /* Populate cluster coordinates */
    i = 0;
    for (y = 0; y < b->size_y; y++) {
      for (x = 0; x < b->size_x; x++) {
        if (clusters[x][y] != cluster_cur) {
          continue;
        }
        xys[i][0] = x;
        xys[i][1] = y;
        i++;
      }
    }

    /* Find tiles set to the same for all valid states  */
    bool first_valid = true;
    do {
      if (is_valid_state(state, cluster_size, xys, b)) {
        for (i = 0; i < cluster_size; i++) {
          if (first_valid) {
            common[i] = state[i];
          } else {
            if (state[i] != common[i]) {
              common[i] = TILE_UNKNOWN;
            }
          }
        }
        first_valid = false;
      }
    } while (next_state(state, cluster_size));

    /* Update board with information */
    for (i = 0; i < cluster_size; i++) {
      x = xys[i][0];
      y = xys[i][1];
      clusters[x][y] = -1;
      if (common[i] != TILE_UNKNOWN) {
        board_changed = true;
        b->adj_mines[x][y] = common[i];
        if (verbose)
          fprintf(stderr, "Constrained (%d, %d) to being %s.\n", x, y,
                  common[i] == TILE_MINE ? "a mine" : "not a mine");
      }
    }

    /* free stuff */
    for (i = 0; i < cluster_size; i++) {
      free(xys[i]);
    }
    free(xys);
    free(state);
    free(common);
  }
  for (x = 0; x < b->size_x; x++) {
    free(clusters[x]);
  }
  free(clusters);

  return board_changed ? OUT_CHANGED : OUT_NOT_CHANGED;
}

char solve_board(struct board *board) {
  struct board *copy;
  bool have_changed_something = false;
  char output;
  int x, y;

  for (x = 0; x < board->size_x; x++) {
    for (y = 0; y < board->size_y; y++) {
      if (board->adj_mines[x][y] == TILE_EXPLOSION) {
        if (fix_found_mines) {
          board->adj_mines[x][y] = TILE_MINE;
          if (verbose)
            fprintf(stderr, "Replaced explosion with mine at (%d,%d)\n", 
                    x, y);
          return OUT_CHANGED;
        } else {
          return OUT_INVALID;
        }
      }
    }
  }

  bool changed = true;
  while (changed) {
    changed = false;
    output = propagate_board(board);

    if (output == OUT_INVALID)
      return output;
    if (output == OUT_CHANGED) {
      changed = true;
      if (verbose)
        fprintf(stderr, "Propagate changed board\n");
    }

    struct neighbor_it *it;
    bool is_interesting;
    for (y = 0; y < board->size_y; y++) {
      for (x = 0; x < board->size_x; x++) {
        if (board->adj_mines[x][y] != TILE_UNKNOWN)
          continue;
        is_interesting = false;
        for(it = new_neighbor_it(board, x, y); next_neighbor_it(it);) {
          if (board->adj_mines[it->x][it->y] != TILE_UNKNOWN)
            is_interesting = true;
        }
        if (!is_interesting)
          continue;
        copy = copy_board(board);
        copy->adj_mines[x][y] = TILE_MINE;
        if (OUT_INVALID == propagate_board(copy)) {
          board->adj_mines[x][y] = TILE_NOT_MINE;
          if (verbose)
            fprintf(stderr, "No mine at (%d,%d)\n", x, y);
          if (OUT_CHANGED == propagate_board(board) && verbose)
            fprintf(stderr, "Propagate changed board\n");
          changed = true;
          continue;
        }
        free_board(copy);

        copy = copy_board(board);
        copy->adj_mines[x][y] = TILE_NOT_MINE;
        if (OUT_INVALID == propagate_board(copy)) {
          board->adj_mines[x][y] = TILE_MINE;
          if (verbose)
            fprintf(stderr, "Mine at (%d,%d)\n", x, y);
          if (OUT_CHANGED == propagate_board(board) && verbose)
            fprintf(stderr, "Propagate changed board\n");
          changed = true;
        }
        free_board(copy);
      }
    }
    if (changed) {
      have_changed_something = true;
      continue;
    }

    output = constraint_satisfaction(board);
    if (output == OUT_CHANGED) {
      have_changed_something = true;
      changed = true;
    }
  }

  return have_changed_something ? OUT_CHANGED : OUT_NOT_CHANGED;
}

/* struct color { */
/*   int red, blue, green; */
/* }; */
/* struct pos { */
/*   int x, y; */
/* }; */

/* Display *display; */
/* void init_get_color() { */
/*   display = XOpenDisplay(NULL); */
/*   if (display == NULL) { */
/*     fprintf(stderr, "Cannot open display\n"); */
/*     exit(1); */
/*   } */
/* } */
/* void get_pixel_color (Display *d, int x, int y, XColor *color) { */
/*   XImage *image; */
/*   image = XGetImage (d, RootWindow (d, DefaultScreen (d)),  */
/*                      x, y, 1, 1, AllPlanes, XYPixmap); */
/*   color->pixel = XGetPixel (image, 0, 0); */
/*   XFree (image); */
/*   XQueryColor (d, DefaultColormap(d, DefaultScreen (d)), color); */
/* } */
/* struct pos get_mouse() { */
/*   XEvent event; */
/*   /\* get info about current pointer position *\/ */
/*   XQueryPointer(display, RootWindow(display, DefaultScreen(display)), */
/*                 &event.xbutton.root, &event.xbutton.window, */
/*                 &event.xbutton.x_root, &event.xbutton.y_root, */
/*                 &event.xbutton.x, &event.xbutton.y, */
/*                 &event.xbutton.state); */

/*   struct pos pos; */
/*   pos.x = event.xbutton.x; */
/*   pos.y = event.xbutton.y; */
/*   return pos; */
/* } */
 
/* struct color get_color() { */
/*   XColor color; */
/*   struct pos p; */

/*   p = get_mouse(); */
/*   get_pixel_color (display, p.x, p.y, &color); */

/*   struct color c; */
/*   c.red = color.red; */
/*   c.blue = color.blue; */
/*   c.green = color.green; */
/*   return c; */
/* } */

/* void move_mouse(int delta_x, int delta_y) { */
/*   Display *display = XOpenDisplay(0); */
/*   Window root = DefaultRootWindow(display); */
/*   struct pos p = get_mouse(); */

/*   XWarpPointer(display, None, root, 0, 0, 0, 0,  */
/*                p.x + delta_x, p.y + delta_y); */
/*   XCloseDisplay(display); */
/* } */

/* char cc(struct color a, struct color b) { */
/*   return a.red == b.red && a.blue == b.blue && a.green == b.green; */
/* } */



/* int main2(int argc, char** args) { */
/*   init_get_color(); */

/*   /\* while (true) { *\/ */
/*   /\*   struct color c = get_color(); *\/ */
/*   /\*   printf("%d %d %d\n", c.red, c.blue, c.green); *\/ */
/*   /\*   sleep(1); *\/ */
/*   /\* } *\/ */

/*   /\* while (true) { *\/ */
/*   /\*   int i; *\/ */
/*   /\*   for (i = 0; i < 200; i++) { *\/ */
/*   /\*     move_mouse(1, 0); *\/ */
/*   /\*   } *\/ */
/*   /\*   for (i = 0; i < 200; i++) { *\/ */
/*   /\*     move_mouse(-1, 0); *\/ */
/*   /\*   } *\/ */
/*   /\*   sleep(1); *\/ */
/*   /\* } *\/ */

/*   /\*  */
/*      unknown box: */
/*         start of box: 60652 59624 60395 */
/*         middle of box: 19275 33667 26985 */
/*         end of box: 35209 31611 34181 */

/*      known box: */
/*         start:  */

/*  *\/ */

/*   struct color c, oc; */
/*   sleep(2); */
/*   c = oc = get_color(); */
/*   printf("%d %d %d\n", c.red, c.blue, c.green); */
/*   while (true) { */
/*     move_mouse(1, 0); */
/*     c = get_color(); */
/*     if (!cc(c, oc)) { */
/*       printf("%d %d %d\n", c.red, c.blue, c.green); */
/*       oc = c; */
/*       sleep(1); */
/*     } */
/*   } */

/*   return 0; */
/* } */

bool guess_board(struct game *game) {
  struct board *b = game->input;
  bool exists_unknown = false;
  int x, y, all_unknowns = 0, all_mines = 0;

  for (x = 0; x < b->size_x; x++) {
    for (y = 0; y < b->size_y; y++) {
      switch (b->adj_mines[x][y]) {
      case TILE_UNKNOWN: 
        exists_unknown = true; 
        all_unknowns++;
        break;
      case TILE_MINE:
        all_mines++;
        break;
      case TILE_EXPLOSION:
      case TILE_NOT_MINE: return false;
      }
    }
  }
  if (!exists_unknown) {
    return false;
  }

  float score_not_mines, score_unknowns, score_mines;
  float score_chance, score_propagate, best_score;

  int best_not_mines = 0, best_unknowns = 0, best_mines = 0,
    best_propagate = 0;
  float best_chance = -1.0;

  int bestx, besty;
  float main_hit_chance, main_score;
  int main_not_mines, main_unknowns, main_mines;
  int unknowns, mines;
  srand((unsigned int) time(NULL));
  do {
    bestx = rand() % b->size_x;
    besty = rand() % b->size_y;
  } while (b->adj_mines[bestx][besty] != TILE_UNKNOWN);
  best_score = FLT_MIN;
  struct neighbor_it *it, *it2;
  for (x = 0; x < b->size_x; x++) {
    for (y = 0; y < b->size_y; y++) {
      if (b->adj_mines[x][y] != TILE_UNKNOWN) {
        continue;
      }
      main_not_mines = 0;
      main_unknowns = 0;
      main_mines = 0;
      main_hit_chance = 1 - ((float) (game->mines - all_mines) / all_unknowns);
      for(it = new_neighbor_it(b, x, y); next_neighbor_it(it);) {
        switch (b->adj_mines[it->x][it->y]) {
        case TILE_UNKNOWN: 
          main_unknowns++;
          break;
        case TILE_MINE:
          main_mines++;
          break;
        }
        if (b->adj_mines[it->x][it->y] >= 0) {
          main_not_mines++;
          unknowns = mines = 0;
          for(it2 = new_neighbor_it(b, it->x, it->y); next_neighbor_it(it2);) {
            switch (b->adj_mines[it2->x][it2->y]) {
            case TILE_UNKNOWN: unknowns++; break;
            case TILE_MINE: mines++; break;
            }
          }
          free_neighbor_it(it2);
          main_hit_chance = min(main_hit_chance,
                                1 - ((float) (b->adj_mines[it->x][it->y] - mines) 
                                     / unknowns));
        }
      }
      score_not_mines = (float) main_not_mines / 8;
      score_unknowns = (float) main_unknowns / 8;
      score_mines = (float) main_mines / 8;
      score_chance = main_hit_chance;
      struct board *temp = copy_board(game->input);
      temp->adj_mines[x][y] = TILE_NOT_MINE;
      score_propagate = propagate_board(temp) == OUT_CHANGED ? 1.0 : 0.0;
      free_board(temp);

      main_score = (score_not_mines * SCORE_MUL_NOT_MINES +
                    score_unknowns * SCORE_MUL_UNKNOWNS +
                    score_mines * SCORE_MUL_MINES +
                    score_chance * SCORE_MUL_CHANCE +
                    score_propagate * SCORE_MUL_PROPAGATE);

      if (main_score > best_score) {
        bestx = x;
        besty = y;
        best_score = main_score;
        best_not_mines = main_not_mines;
        best_unknowns = main_unknowns;
        best_mines = main_mines;
        best_chance = main_hit_chance;
        best_propagate = (int) score_propagate;
      }
    }
  }
  if (verbose) {
    fprintf(stderr, "Guessed (%d,%d) is not a mine (score: %.2f).\n",
            bestx, besty, best_score);
    fprintf(stderr, "not mines=%d  mines=%d  unknowns=%d  propagate=%d  "
            "chance=%.2f\n",
            best_not_mines, best_mines, best_unknowns, best_propagate,
            best_chance);
  }
  b->adj_mines[bestx][besty] = TILE_NOT_MINE;
  return true;
}

void print_help() {
  printf("Usage: minesweeper_solver [FILE]\n"
         "Solve as much as possible of a minesweeper game\n"
         "from FILE or standard input. It solves it by\n"
         "replacing unknown tiles (.) with either mines (!)\n"
         "or not-mines (?).\n"
         "\n"
         "  -h, --help  Show this help menu\n"
         "  -v          Verbose output (to stderr)\n"
         "  -m          Replace explosions (*) with mines (!)\n"
         "  -c INT      Set new cluster size limit (default is %d)\n"
         "\n"
         "The game modifies a interface file generated by\n"
         "'minesweeper'.\n"
         "\n"
         "Example: Solve a game of minesweeper\n"
         "  Start a new game called foo using the 'minesweeper'\n"
         "    program.\n"
         "  Solve minesweeper:\n"
         "    1) run 'minesweeper_solver foo'\n"
         "    2) Process a turn by running 'minesweeper foo'\n"
         "    3) Go to step 1)\n",
         max_cluster_size
         );
}

int solve(int argc, char** args) {
  struct game *game;

  if (argc > 1 &&
      (0 == strcmp("-h", args[1]) ||
       0 == strcmp("--help", args[1]))) {
    print_help();
    return 0;
  }

  int i = 1;
  char *file = NULL;
  while (i < argc) {
    if (args[i][0] == '-') {
      switch (args[i][1]) {
      case 'v': verbose = true; break;
      case 'm': fix_found_mines = true; break;
      case 'c': max_cluster_size = atoi(args[i+1]); i++; break;
      default: fprintf(stderr, "Unknown argument '%s'\n", args[i]); return 1;
      }
    } else {
      file = args[i];
    }
    i++;
  }
  FILE *f;
  if (NULL == file || 0 == strcmp(file, "-")) {
    f = stdin;
  } else {
    f = fopen(file, "r");
  }  
  game = load_game(f);
  fclose(f);

  if (NULL == game) {
    fprintf(stderr, "minesweeper_solver: invalid game.\n");
    return 1;
  }

  bool board_changed = false;

  char out = solve_board(game->input);
  switch (out) {
  case OUT_INVALID:
    if (verbose)
      fprintf(stderr, "Board can't be solved.\n");
    return 0;
    break;
  case OUT_CHANGED:
    board_changed = true;
    break;
  }

  if (!board_changed) {
    board_changed = guess_board(game);
  }

  if (!board_changed && verbose) {
    fprintf(stderr, "Nothing to do.\n");
    return 0;
  }

  if (NULL == file || 0 == strcmp(file, "-")) {
    f = stdout;
  } else {
    f = fopen(file, "w");
  }
  save_game(game, f);
  fclose(f);  

  return 0;
}

int main(int argc, char** args) {
  return solve(argc, args);
}
