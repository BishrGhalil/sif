CC=clang
CFLAGS=-Wall -Wextra -std=c11 -ggdb
LIBS=
SRC=sif.c recdir.c

all: main
main: $(SRC)
	$(CC) $(CFLAGS) $(SRC) $(LIBS) -o sif

clean:
	rm -rf sif

