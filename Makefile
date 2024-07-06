CC := gcc
CFLAGS := -O3 -s -std=c23 -Wall -Wextra -pedantic
CFLAGS_DEBUG := -g -std=c23 -Wall -Wextra -pedantic

all: compile

compile: src/comp.c
	${CC} ${CFLAGS} $? -o comp

debug: src/comp.c
	${CC} ${CFLAGS_DEBUG} -D DEBUG $? -o comp
