#ifndef IPC_H
#define IPC_H

#define DIRECTION_FORWARD 1
#define DIRECTION_BACKWARD -1

typedef struct {
    int piece_id;
    int team_id;
    int direction;
} FurniturePiece;

void print_piece(FurniturePiece piece);

#endif