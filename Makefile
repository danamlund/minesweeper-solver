
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
	-lncurses \
	-o minesweeper_curses

clean:
	rm minesweeper minesweeper_solver minesweeper_curses
