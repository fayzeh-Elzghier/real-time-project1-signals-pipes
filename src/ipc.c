#include <stdio.h>
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