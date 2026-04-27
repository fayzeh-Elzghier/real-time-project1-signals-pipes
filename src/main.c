#include <stdio.h>
#include "config.h"
#include "ipc.h"
#include "team.h"

int main(int argc, char *argv[]) {
    const char *config_file = "config.txt";

    if (argc > 1) {
        config_file = argv[1];
    }

    Config config = read_config(config_file);
    print_config(config);

    printf("\nTesting FurniturePiece:\n");

    FurniturePiece test_piece;
    test_piece.piece_id = 3;
    test_piece.team_id = 1;
    test_piece.direction = DIRECTION_FORWARD;

    print_piece(test_piece);

    test_team_setup(config);

    return 0;
}