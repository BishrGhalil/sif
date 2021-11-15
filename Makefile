CC=gcc
CFLAGS=-Wall -std=c11 -ggdb
LIBS=
SRC=src/sif.c src/recdir.c src/argparse.c

all: main
main: $(SRC)
	$(CC) $(CFLAGS) $(SRC) $(LIBS) -o sif
install:
	cp sif /usr/bin/sif

uninstall:
	rm -rf /usr/bin/sif
clean:
	rm -rf sif

