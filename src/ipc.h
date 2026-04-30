#ifndef IPC_H
#define IPC_H

#define DIRECTION_FORWARD 1
#define DIRECTION_BACKWARD -1

#define STATUS_ACCEPTED 1
#define STATUS_WRONG 0

typedef struct {
    int piece_id;
    int team_id;
    int direction;
} FurniturePiece;

typedef struct {
    int status;
    FurniturePiece piece;
} ReturnMessage;

void print_piece(FurniturePiece piece);
void print_return_message(ReturnMessage message);

#endif