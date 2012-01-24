/* Dan Amlund Thomsen
   20 Jan. 2012
   dan@danamlund.dk
   www.danamlund.dk */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

/* #include <X11/Xlib.h> */
/* #include <X11/Xutil.h> */

#include "minesweeper_lib.h"

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
          changed = true;
          if (verbose)
            fprintf(stderr, "No mine at (%d,%d)\n", x, y);
          continue;
        }
        free_board(copy);

        copy = copy_board(board);
        copy->adj_mines[x][y] = TILE_NOT_MINE;
        if (OUT_INVALID == propagate_board(copy)) {
          board->adj_mines[x][y] = TILE_MINE;
          changed = true;
          if (verbose)
            fprintf(stderr, "Mine at (%d,%d)\n", x, y);
        }
        free_board(copy);
      }
    }
    if (changed)
      have_changed_something = true;
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

  int bestx, besty;
  int best_known_neighbours = 0;
  float best_hit_chance, new_hit_chance;
  int unknowns, mines, known_neighbours;
  srand((unsigned int) time(NULL));
  do {
    bestx = rand() % b->size_x;
    besty = rand() % b->size_y;
  } while (b->adj_mines[bestx][besty] != TILE_UNKNOWN);
  best_hit_chance = (game->mines - all_mines) / all_unknowns;
  best_hit_chance = max(0.3, best_hit_chance);
  struct neighbor_it *it, *it2;
  for (x = 0; x < b->size_x; x++) {
    for (y = 0; y < b->size_y; y++) {
      if (b->adj_mines[x][y] == TILE_UNKNOWN) {
        known_neighbours = 0;
        new_hit_chance = 0;
        for(it = new_neighbor_it(b, x, y); next_neighbor_it(it);) {
          if (b->adj_mines[it->x][it->y] >= 0) {
            known_neighbours++;
            unknowns = mines = 0;
            for(it2 = new_neighbor_it(b, it->x, it->y); next_neighbor_it(it2);) {
              switch (b->adj_mines[it2->x][it2->y]) {
              case TILE_UNKNOWN: unknowns++; break;
              case TILE_MINE: mines++; break;
              }
            }
            free_neighbor_it(it2);
            new_hit_chance = max(new_hit_chance,
                                 (float) (b->adj_mines[it->x][it->y] - mines) 
                                 / unknowns);
          }
        }
        if (known_neighbours > 0 &&
            (new_hit_chance < best_hit_chance ||
             (new_hit_chance == best_hit_chance &&
              known_neighbours > best_known_neighbours))) {
            bestx = x;
            besty = y;
            best_hit_chance = new_hit_chance;
            best_known_neighbours = known_neighbours;          
        }
      }
    }
  }
  if (verbose)
    fprintf(stderr, "Guessed (%d,%d) is not a mine (%.2f%%).\n",
            bestx, besty, best_hit_chance);
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
         "  -v          Print changes\n"
         "  -m          Replace explosions (*) with mines (!)\n"
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
         "    3) Go to step 1)\n"
         );
}

int solve(int argc, char** args) {
  struct game *game;
  char out;

  if (argc > 1 &&
      (0 == strcmp("-h", args[1]) ||
       0 == strcmp("--help", args[1]))) {
    print_help();
    exit(0);
  }

  int i = 1;
  char *file = NULL;
  while (i < argc) {
    if (args[i][0] == '-') {
      switch (args[i][1]) {
      case 'v': verbose = true; break;
      case 'm': fix_found_mines = true; break;
      default: fprintf(stderr, "Unknown argument '%s'\n", args[i]); exit(1);
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
    exit(1);
  }

  out = solve_board(game->input);
  if (OUT_INVALID == out) {
    if (verbose)
      fprintf(stderr, "Board can't be solved.\n");
  } else if (OUT_NOT_CHANGED == out) {
    if (!guess_board(game)) {
      if (verbose)
        fprintf(stderr, "Nothing to do.\n");
      exit(0);
    }
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
