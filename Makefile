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

package_linux: minesweeper minesweeper_solver minesweeper_curses
	rm -Rf minesweeper-solver
	mkdir minesweeper-solver
	cp minesweeper minesweeper_solver minesweeper_curses README \
	   minesweeper-solver
	mkdir minesweeper-solver/src
	cp minesweeper.c minesweeper_lib.c minesweeper_lib.h \
	   minesweeper_curses.c minesweeper_solver.c Makefile \
	   minesweeper-solver/src
	tar czvf minesweeper-solver.tar.gz minesweeper-solver

clean:
	rm -f minesweeper minesweeper_solver minesweeper_curses
