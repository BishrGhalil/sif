CC=clang
CFLAGS=-Wall -std=c11 -ggdb
LIBS=
SRC=sif.c recdir.c argparse.c

all: main
main: $(SRC)
	$(CC) $(CFLAGS) $(SRC) $(LIBS) -o sif

clean:
	rm -rf sif
