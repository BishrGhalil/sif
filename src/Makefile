CC=gcc
CFLAGS=-Wall -std=c11 -ggdb
LIBS=
SRC=sif.c recdir.c argparse.c
OUTPUT=sif

all: main
main: $(SRC)
	$(CC) $(CFLAGS) $(SRC) $(LIBS) -o $(OUTPUT)

clean:
	rm -rf $(OUTPUT)
