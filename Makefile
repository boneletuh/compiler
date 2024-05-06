all: run

run: src/comp.o
	gcc -O3 -s -Wall -Wextra -pedantic src/comp.o -o comp

clean:
	rm src/*.o