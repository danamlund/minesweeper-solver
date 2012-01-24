ifdef SystemRoot
  CURSESLIB = pdcurses
else
  CURSESLIB = ncurses
endif

all: minesweeper minesweeper_solver minesweeper_curses

minesweeper: minesweeper_lib.c minesweeper.c
	gcc -Wall -O3 \
	minesweeper_lib.c minesweeper.c \
	-o minesweeper


minesweeper_solver: minesweeper_lib.c minesweeper_solver.c
	gcc -Wall -O3 \
	minesweeper_lib.c minesweeper_solver.c \
	-o minesweeper_solver

minesweeper_curses: minesweeper_lib.c minesweeper_curses.c
	gcc -Wall -O3 \
	minesweeper_curses.c \
	-l$(CURSESLIB) \
	-o minesweeper_curses

clean:
	rm -f minesweeper minesweeper_solver minesweeper_curses
