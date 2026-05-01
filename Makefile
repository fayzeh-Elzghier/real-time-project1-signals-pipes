CC = gcc
CFLAGS = -Wall -Wextra -g -fopenmp
SRC = src/main.c src/config.c src/ipc.c src/team.c src/graphics.c
OUT = project1
LIBS = -lglut -lGLU -lGL

all:
	$(CC) $(CFLAGS) $(SRC) -o $(OUT) $(LIBS)

clean:
	rm -f $(OUT)