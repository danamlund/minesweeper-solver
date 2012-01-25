/* Dan Amlund Thomsen
   20 Jan. 2012
   dan@danamlund.dk
   www.danamlund.dk/minesweeper_solver */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <curses.h>

#ifdef __unix__ /* __unix__ is usually defined by compilers targeting
                   Unix systems */
#define FILE_PREFIX "./"
#elif defined _WIN32 /* _Win32 is usually defined by compilers
                        targeting 32 or 64 bit Windows systems */
#define FILE_PREFIX ""
#endif

#define true 1
#define false 0
#define min(X, Y)  ((X) < (Y) ? (X) : (Y))
#define max(X, Y)  ((X) > (Y) ? (X) : (Y))

#define COL_1 1
#define COL_2 2
#define COL_3 3
#define COL_4 4
#define COL_5 5
#define COL_6 6
#define COL_7 7
#define COL_8 8
#define COL_UNKNOWN 9
#define COL_MINE 10
#define COL_EXPLOSION 11

static char *window_id = NULL;
static int screenshot_id = 0;

static char *file_minesweeper = FILE_PREFIX "minesweeper";
static char *file_minesweeper_solver = FILE_PREFIX "minesweeper_solver -m";
static char *file_suffix = "";

struct game {
  int x, y, mines;
  char **input;
  int seed;
  char **solution;
};

void init_curses() {
  initscr(); cbreak(); noecho(); 
  /* curs_set(0); */ keypad(stdscr, TRUE);
  start_color();

  init_pair(COL_1, COLOR_BLUE, COLOR_BLACK);
  init_pair(COL_2, COLOR_GREEN, COLOR_BLACK);
  init_pair(COL_3, COLOR_MAGENTA, COLOR_BLACK);
  init_pair(COL_4, COLOR_YELLOW, COLOR_BLACK);
  init_pair(COL_5, COLOR_CYAN, COLOR_BLACK);
  init_pair(COL_6, COLOR_CYAN, COLOR_BLACK);
  init_pair(COL_7, COLOR_CYAN, COLOR_BLACK);
  init_pair(COL_8, COLOR_CYAN, COLOR_BLACK);

  init_pair(COL_UNKNOWN, COLOR_WHITE, COLOR_BLACK);
  init_pair(COL_MINE, COLOR_RED, COLOR_BLACK);
  init_pair(COL_EXPLOSION, COLOR_YELLOW, COLOR_RED);
}
void exit_curses(int ret) {
  endwin();
  exit(ret);
}
void err(char *str) {
  erase();
  printw(str);
  refresh();
  getch();
}

void draw_box(int x1, int y1, int x2, int y2) {
  int x, y;

  for (x = x1; x <= x2; x++) {
    mvaddch(y1, x, '-');
    mvaddch(y2, x, '-');
  }

  for (y = y1; y <= y2; y++) {
    mvaddch(y, x1, '|');
    mvaddch(y, x2, '|');
  }
  mvaddch(y1, x1, '/');
  mvaddch(y1, x2, '\\');
  mvaddch(y2, x1, '\\');
  mvaddch(y2, x2, '/');
}

struct game* fread_game(FILE *f) {
  int x, y, sx, sy, mines, seed;
  if (3 != fscanf(f, "Minesweeper %dx%d grid with %d mines",
                  &sx, &sy, &mines)) {
    err("minesweeper_curses: invalid game.\n");
    exit_curses(1);
  }
  char **input = (char **) malloc(sizeof(char*)*sx);
  char **solution = (char **) malloc(sizeof(char*)*sx);
  fgetc(f);
  for (x = 0; x < sx; x++) {
    input[x] = (char *) malloc(sizeof(char)*sy);
    solution[x] = (char *) malloc(sizeof(char)*sy);
  } 
  for (y = 0; y < sy; y++) {
    for (x = 0; x < sx; x++) {
      input[x][y] = fgetc(f);
    }
    fgetc(f);
  }
  fgetc(f);
  if (1 != fscanf(f, "Solution encrypted with seed %d", &seed)) {
    err("minesweeper_curses: invalid game.\n");
    exit_curses(1);
  }
  fgetc(f);
  for (y = 0; y < sy; y++) {
    for (x = 0; x < sx; x++) {
      solution[x][y] = fgetc(f);
    }
    fgetc(f);
  }

  struct game *game = (struct game*) malloc(sizeof(struct game));
  game->x = sx;
  game->y = sy;
  game->mines = mines;
  game->input = input;
  game->seed = seed;
  game->solution = solution;
  return game;
}

struct game* read_game(char *file) {
  FILE *f = fopen(file, "r");
  struct game *game = fread_game(f);
  fclose(f);
  return game;
}  

void fsave_game(FILE *f, struct game *game) {
  fprintf(f, "Minesweeper %dx%d grid with %d mines\n",
          game->x, game->y, game->mines);

  int x, y;
  for (y = 0; y < game->y; y++) {
    for (x = 0; x < game->x; x++) {
      fputc(game->input[x][y], f);
    }
    fputc('\n', f);
  }
  fputc('\n', f);
  fprintf(f, "Solution encrypted with seed %d\n", game->seed);
  for (y = 0; y < game->y; y++) {
    for (x = 0; x < game->x; x++) {
      fputc(game->solution[x][y], f);
    }
    fputc('\n', f);
  }
}

void save_game(struct game *game, char* file) {
  FILE *f = fopen(file, "w+");
  fsave_game(f, game);
  fclose(f);
}

void free_game(struct game *game) {
  int x;
  for (x = 0; x < game->x; x++) {
    free(game->input[x]);
  }
  free(game);
}

void print_file(char *filename, int y, int x) {
  FILE *f = fopen(filename, "r");
  int yy = y;
  char c;

  move(y, x);
  while (EOF != (c = fgetc(f))) {
    if ('\n' == c) {
      yy++;
      move(yy, x);
    } else {
      printw("%c", c);
    }
  }
}

void free_char_board(char **b, int size_x) {
  int i;
  for (i = 0; i < size_x; i++) {
    free(b[i]);
  }
  free(b);
}

char** read_file(char *filename, int size_x, int size_y) {
  int i;
  char **b;
  FILE *f = fopen(filename, "r");
  int x = 0, y = 0;
  char c;

  b = (char **) malloc(sizeof(char*)*size_x);
  for (i = 0; i < size_x; i++) {
    b[i] = (char*) malloc(sizeof(char)*size_y);
  }

  while (EOF != (c = fgetc(f))) {
    if ('\n' == c) {
      y++;
      x = 0;
    } else {
      b[x][y] = c;
      x++;
    }
  }
  return b;
}

void save_char_board(char **b, int size_x, int size_y, char *filename) {
  FILE *f = fopen(filename, "w");
  int x, y;

  for (y = 0; y < size_y; y++) {
    for (x = 0; x < size_x; x++) {
      fputc(b[x][y], f);
    }
    fputc('\n', f);
  }
  fclose(f);
}

void my_exec(char *prog, char *file, char *suffix) {
  char cmd[1024];
  sprintf(cmd, "%s %s %s", prog, file, suffix);
  int suppress_warning = system(cmd);
  if (suppress_warning);
}
void process_turn(char *file) {
  my_exec(file_minesweeper, file, file_suffix);
}
void solve(char *file) {
  my_exec(file_minesweeper_solver, file, file_suffix);
}
void screenshot(char *file) {
  if (NULL == window_id) {
    return;
  }
  char cmd[1024];
  sprintf(cmd, "import -window %s screenshot_%s_%03d.png", 
          window_id, file, screenshot_id);
  screenshot_id++;
  int suppress_warning = system(cmd);
  if (suppress_warning);
}

void show_help() {
  erase();
  printw("\n"
         "\n"
         "\n"
         "             =======  Keys  =======\n"
         "                       q: quit\n"
         "              Arrow keys: move cursor\n"
         "    Home/End/PgUp/PgDown: move cursor to edges\n"
         "              Enter or ?: Set tile as not a mine\n"
         "              Space or !: Define tile as being a mine\n"
         "                       a: Automatically solve it\n"
         "                       s: Solve one turn\n"
         "                       w: Have solver fill out tiles\n"
         "                       e: Process solver's filled out tiles\n"
         "                       0: Start a new easy game\n"
         "                       1: Start a new normal game\n"
         "                       2: Start a new expert game\n"
         "                       3: Start a new insane game\n"
         "                       h: Show help\n"
         );
  refresh();
  getch();
}

void print_board(struct game *game, int start_y, int start_x) {
  int c;
  int x, y;
  for (x = 0; x < game->x; x++) {
    for (y = 0; y < game->y; y++) {
      switch (game->input[x][y]){
      case '1': case '2': case '3': case '4': 
      case '5': case '6': case '7': case '8':
        c = game->input[x][y] - '0'; break;
      case '0': 
      case '?':
      case '.': c = COL_UNKNOWN; break;
      case '!': c = COL_MINE; break;
      case '*': c = COL_EXPLOSION; break;
      default: c = COL_UNKNOWN;
      }
      attron(COLOR_PAIR(c));
      mvaddch(start_y + y, start_x + x, game->input[x][y]);
      attroff(COLOR_PAIR(c));
    }
  }
}

void print_ui(char *file, struct game *game, int yadd, int xadd) {
  erase();
  move(0, 0);
  printw("Minesweeper - %s - %dx%d - %d mines - h: help", 
         file, game->x, game->y, game->mines);

  draw_box(xadd - 1, yadd - 1, game->x + xadd, game->y + yadd);
  print_board(game, yadd, xadd);
  move(yadd + game->y / 2, xadd + game->x / 2);
  refresh();
}

void start_new_game(char *file, int type) {
  char args[100];
  sprintf(args, "-n -b %d", type);
  my_exec(file_minesweeper, file, args);
}

void game(char* file) {
  struct game *game = read_game(file);

  int x = game->x/2;
  int y = game->y/2;

  int c, run = true;
  int xadd = 1, yadd = 2;
  char board_changed = false;

  print_ui(file, game, yadd, xadd);
  while (run) {
    c = getch();
    switch (c) {
    case KEY_LEFT: x = max(x-1, 0); break;
    case KEY_RIGHT: x = min(x+1, game->x - 1); break;
    case KEY_UP: y = max(y-1, 0); break;
    case KEY_DOWN: y = min(y+1, game->y - 1); break;
    case KEY_HOME: x = 0; break;
    case KEY_END: x = game->x - 1; break;
    case KEY_PPAGE: y = 0; break;
    case KEY_NPAGE: y = game->y - 1; break;
    case '\n':
    case '?': 
      if (game->input[x][y] != '!') {
        game->input[x][y] = '?';
        board_changed = true;
      }
      break;
    case ' ':
    case '!': 
      game->input[x][y] = '!'; 
      attron(COLOR_PAIR(COL_MINE));
      mvaddch(y+yadd, x+xadd, '!'); 
      attroff(COLOR_PAIR(COL_MINE)); 
      break;
    case '.': 
      game->input[x][y] = '.'; 
      attron(COLOR_PAIR(COL_UNKNOWN));
      mvaddch(y+yadd, x+xadd, '.'); 
      attroff(COLOR_PAIR(COL_UNKNOWN)); 
      break;
    case 's':
      save_game(game, file);
      solve(file);
      process_turn(file);
      free_game(game);
      game = read_game(file);
      print_board(game, yadd, xadd);
      break;
    case 'w':
      save_game(game, file);
      solve(file);
      free_game(game);
      game = read_game(file);
      print_board(game, yadd, xadd);
      break;
    case 'e':
      save_game(game, file);
      process_turn(file);
      free_game(game);
      game = read_game(file);
      print_board(game, yadd, xadd);
      break;
    case 'a':
      if (window_id) {
        timeout(1);
      } else {
        timeout(500);
      }
      curs_set(0);
      refresh();
      save_game(game, file);
      screenshot(file);
      while (true) {
        solve(file);
        free_game(game);
        game = read_game(file);
        print_board(game, yadd, xadd);
        refresh();
        screenshot(file);
        if (getch() != ERR) break;

        process_turn(file);
        free_game(game);
        game = read_game(file);
        print_board(game, yadd, xadd);
        refresh();
        screenshot(file);
        if (getch() != ERR) break;
      }
      curs_set(1);
      timeout(-1);
      refresh();
      break;
    case 'q': run = 0; break;
    case '0': case '1': case '2': case '3':
      start_new_game(file, c - '0');
      free_game(game);
      game = read_game(file);

      x = game->x/2;
      y = game->y/2;

      print_ui(file, game, yadd, xadd);
      print_board(game, yadd, xadd);
      board_changed = true;
      break;
    case 'h': 
      show_help(); 
      print_ui(file, game, yadd, xadd);
      break;
    }
    if (board_changed) {
      save_game(game, file);
      process_turn(file);
      free_game(game);
      game = read_game(file);
      print_board(game, yadd, xadd);
      board_changed = false;
    }
    move(y+yadd, x+xadd);
  }
}

bool file_exists(char *file) {
  FILE *f = fopen(file, "r");
  if (f != NULL) {
    fclose(f);
    return true;
  }
  return false;
}

void print_help() {
  printf("Usage: minesweeper_curses [FILE]\n"
         "Start a curses user interface using the minesweeper\n"
         "game from FILE or game.txt by default.\n"
         "\n"
         "  -h, --help   Show this help menu.\n"
         "  -w WINDOW-ID Set up recording using Imagemagick\n"
         "  -m 'CMD'     Use CMD instead of './minesweeper'\n"
         "  -s 'CMD'     Use CMD instead of './minesweeper_solver -m'\n"
         "  -a 'ARGS'    Append ARGS when running './minesweeper'\n"
         "                 and './minesweeper_solver'\n");
}

int run(int argc, char **args) {
  if (argc > 1 &&
      (0 == strcmp("-h", args[1]) ||
       0 == strcmp("--help", args[1]))) {
    print_help();
    return 0;
  }

  char *file = NULL;
  int i;
  for (i = 1; i < argc; i++) {
    if (args[i][0] == '-') {
      switch (args[i][1]) {
      case 'w': 
        window_id = args[i+1];
        i++;
        break;
      case 'm':
        file_minesweeper = args[i+1];
        i++;
        break;
      case 's':
        file_minesweeper_solver = args[i+1];
        i++;
        break;
      case 'a':
        file_suffix = args[i+1];
        i++;
        break;
      default:
        printf("minesweeper_curses: unknown argument: '-%c'\n", args[i][1]);
        return 1;
      }
    } else {
      file = args[i];
    }
  }

  printf("1\n");
  if (NULL != file && !file_exists(file)) {
    printf("minesweeper_curses: File '%s' doesn't exists.\n", file);
    return 1;
  }

  if (NULL == file) {
    file = "game.txt";
    if (!file_exists(file)) {
      start_new_game(file, 1);
    }
  }

  init_curses();
  game(file);
  exit_curses(0);
  return 0;
}

int main(int argc, char **args) {
  return run(argc, args);

  /* attron(COLOR_PAIR(COL_1)); */
  /* /\* attrset(COLOR_PAIR(COL_1)); *\/ */
  /* mvaddch(0, 0, '!'); */
  /* /\* printw("foo"); *\/ */
  /* /\* attrset(A_NORMAL); *\/ */
  /* attroff(COLOR_PAIR(COL_1)); */
  /* mvaddch(1, 0, '!'); */
  /* /\* printw("bar"); *\/ */
  /* getch(); */

  /* print_file("minesweeper_foo.txt", 2, 2); */
  /* draw_box(1, 1, 22, 22); */
  /* control(1, 1, 22, 22); */


  exit_curses(0);
  return 0;
}
