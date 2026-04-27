CC = gcc
CFLAGS = -Wall -Wextra -g
SRC = src/main.c src/config.c src/ipc.c src/team.c
OUT = project1

all:
	$(CC) $(CFLAGS) $(SRC) -o $(OUT)

clean:
	rm -f $(OUT)