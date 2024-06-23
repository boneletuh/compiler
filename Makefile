CC := gcc
CFLAGS := -O3 -s -Wall -Wextra -pedantic

all: run

run: src/comp.c
	${CC} ${CFLAGS} $? -o src/comp
