CC = gcc -g -DCSE320
CFLAGS = -Wall -Werror
BIN = utfconverter

SRC = $(wildcard *.c)

all: $(BIN)

$(BIN): $(SRC)
	$(CC) $(CFLAGS) $^ -o utfconverter

clean:
	rm -f *.o $(BIN)

