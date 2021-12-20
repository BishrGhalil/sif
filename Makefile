CC=gcc
CFLAGS=-Wall -std=c11 -ggdb
LIBS=-lpcre
SRC=src/sif.c src/recdir.c src/argparse.c
OUTPUT=sif

all: main
main: $(SRC)
	$(CC) $(CFLAGS) $(SRC) $(LIBS) -o $(OUTPUT)
install:
	cp $(OUTPUT) /usr/bin/$(OUTPUT)

uninstall:
	rm -rf /usr/bin/$(OUTPUT)
clean:
	rm -rf $(OUTPUT)

