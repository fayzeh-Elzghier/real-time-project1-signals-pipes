#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "ipc.h"
void print_piece(FurniturePiece piece) {
    const char *direction_text;

    if (piece.direction == DIRECTION_FORWARD) {
        direction_text = "FORWARD";
    } else if (piece.direction == DIRECTION_BACKWARD) {
        direction_text = "BACKWARD";
    } else {
        direction_text = "UNKNOWN";
    }

    printf("Team %d | Piece %d | Direction: %s\n",
           piece.team_id,
           piece.piece_id,
           direction_text);
}

void print_return_message(ReturnMessage message) {
    const char *status_text;

    if (message.status == STATUS_ACCEPTED) {
        status_text = "ACCEPTED";
    } else if (message.status == STATUS_WRONG) {
        status_text = "WRONG";
    } else {
        status_text = "UNKNOWN";
    }

    printf("Return Status: %s | ", status_text);
    print_piece(message.piece);
}
void log_event(const char *message) {
    int fd = open(FIFO_PATH, O_WRONLY | O_NONBLOCK);

    if (fd == -1) {
        return;
    }

    write(fd, message, strlen(message));
    write(fd, "\n", 1);

    close(fd);
}