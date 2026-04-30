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

    create_team_processes(config, 1);
    create_team_processes(config, 2);
    //test_pipe_between_two_members();
    //test_pipe_chain_for_team(config, 1);
    //test_sink_checking(config, 1);
    //test_backward_return(config, 1);
    //test_wrong_then_correct(config, 1);
    //test_source_piece_selection(config, 1);
    //test_selection_with_pipes(config, 1);
    //test_status_pipe(config, 1);
    //test_loop_until_accepted(config, 1);
    //test_return_message_protocol(config, 1);
    //test_accept_all_pieces_with_return_message(config, 1);
    //test_random_selection_with_return_message(config, 1);
    test_random_selection_with_blocked_piece(config, 1);
    return 0;
}