#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>
#include "team.h"
#include "ipc.h"
#include <time.h>
#include <omp.h>
#include <unistd.h>
unsigned int make_random_seed(int team_id, int member_id) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);

    return (unsigned int)(
        ts.tv_nsec ^
        ts.tv_sec ^
        getpid() ^
        (team_id * 1000) ^
        (member_id * 100)
    );
}

void random_member_delay(Config config, int moved_count, int team_id, int member_id) {
    int range = config.max_delay - config.min_delay + 1;
    int base_delay = config.min_delay;

    if (range > 0) {
        base_delay = config.min_delay + (rand() % range);
    }

    /*
     * Tiredness factor:
     * every 5 moves, add 1 extra second.
     */
    int tiredness_delay = moved_count / 20;

    int total_delay = base_delay + tiredness_delay;

    printf("Team %d Member %d delaying for %d seconds (move count = %d)\n",
           team_id,
           member_id,
           total_delay,
           moved_count);

    sleep(total_delay);
}
void test_random_selection_with_blocked_piece(Config config, int team_id) {
    int num_members = config.team_members;
    int num_pieces = config.furniture_pieces;

    int forward_pipes[num_members - 1][2];
    int backward_pipes[num_members - 1][2];

    printf("\nTesting Random Selection With Blocked Piece + Delay for Team %d:\n", team_id);

    for (int i = 0; i < num_members - 1; i++) {
        if (pipe(forward_pipes[i]) == -1) {
            perror("forward pipe failed");
            exit(1);
        }

        if (pipe(backward_pipes[i]) == -1) {
            perror("backward pipe failed");
            exit(1);
        }
    }

    for (int member_id = 0; member_id < num_members; member_id++) {
        pid_t pid = fork();

        if (pid < 0) {
            perror("fork failed");
            exit(1);
        }

        if (pid == 0) {
            //srand(getpid() + time(NULL));
            srand(make_random_seed(team_id, member_id));
            if (member_id == 0) {
                int accepted_count = 0;
                int available[num_pieces];
                int blocked_piece = -1;
                int moved_count = 0;
                #pragma omp parallel for
                for (int i = 0; i < num_pieces; i++) {
                    available[i] = 1;
                }

                while (accepted_count < num_pieces) {
                    int selected_index;
                    int selected_piece_id;

                    do {
                        selected_index = rand() % num_pieces;
                        selected_piece_id = selected_index + 1;

                        if (selected_piece_id == blocked_piece) {
                            printf("Team %d Source tried to select blocked piece %d, so it was rejected and another piece will be selected.\n",
                                   team_id,
                                   blocked_piece);
                        }

                    } while (available[selected_index] == 0 ||
                             selected_piece_id == blocked_piece);

                    FurniturePiece piece;
                    piece.piece_id = selected_piece_id;
                    piece.team_id = team_id;
                    piece.direction = DIRECTION_FORWARD;

                    printf("Team %d Source Member %d randomly selected piece:\n",
                           team_id,
                           member_id);
                    print_piece(piece);
                    char log_message[128];
                    snprintf(log_message, sizeof(log_message),
                    "[Team %d] Source selected piece %d.",
                    team_id,
                    piece.piece_id);
                    log_event(log_message);
                    moved_count++;
                    random_member_delay(config, moved_count, team_id, member_id);

                    write(forward_pipes[0][1],
                          &piece,
                          sizeof(FurniturePiece));

                    ReturnMessage response;

                    read(backward_pipes[0][0],
                         &response,
                         sizeof(ReturnMessage));

                    printf("Team %d Source Member %d received response:\n",
                           team_id,
                           member_id);
                    print_return_message(response);

                    if (response.status == STATUS_ACCEPTED) {
                        available[response.piece.piece_id - 1] = 0;
                        accepted_count++;
                        blocked_piece = -1;

                        printf("Team %d Source accepted count = %d/%d\n",
                               team_id,
                               accepted_count,
                               num_pieces);
                    } else if (response.status == STATUS_WRONG) {
                        blocked_piece = response.piece.piece_id;

                        printf("Team %d Source will keep piece %d available, but block it for the next selection.\n",
                               team_id,
                               response.piece.piece_id);
                    }
                }

                printf("Team %d finished all furniture pieces using random selection with blocked piece rule.\n",
                       team_id);

            } else if (member_id == num_members - 1) {
                int expected_piece = 1;
                int moved_count = 0;

                while (expected_piece <= num_pieces) {
                    FurniturePiece received_piece;

                    read(forward_pipes[member_id - 1][0],
                         &received_piece,
                         sizeof(FurniturePiece));

                    printf("Team %d Sink Member %d received piece. Expected = %d:\n",
                           team_id,
                           member_id,
                           expected_piece);
                    print_piece(received_piece);

                    ReturnMessage response;
                    response.piece = received_piece;

                    if (received_piece.piece_id == expected_piece) {
                        response.status = STATUS_ACCEPTED;

                        printf("Result: ACCEPTED. Piece %d is correct.\n",
                               received_piece.piece_id);
                        char log_message[128];
                        snprintf(log_message, sizeof(log_message),
                                "[Team %d] Sink accepted piece %d.",
                                team_id,
                                received_piece.piece_id);
                        log_event(log_message);
                        expected_piece++;
                    } else {
                        response.status = STATUS_WRONG;
                        response.piece.direction = DIRECTION_BACKWARD;

                        printf("Result: WRONG. Expected piece %d but received piece %d.\n",
                               expected_piece,
                               received_piece.piece_id);
                                char log_message[128];
                                snprintf(log_message, sizeof(log_message),
                                "[Team %d] Sink rejected piece %d, expected %d.",
                                team_id,
                                received_piece.piece_id,
                                expected_piece);
                                log_event(log_message);
                    }

                    printf("Team %d Sink Member %d sending response backward:\n",
                           team_id,
                           member_id);
                    print_return_message(response);

                    moved_count++;
                    random_member_delay(config, moved_count, team_id, member_id);

                    write(backward_pipes[member_id - 1][1],
                          &response,
                          sizeof(ReturnMessage));
                }

            } else {
                int finished = 0;
                int moved_count = 0;

                while (!finished) {
                    FurniturePiece forward_piece;

                    ssize_t bytes_read = read(forward_pipes[member_id - 1][0],
                                              &forward_piece,
                                              sizeof(FurniturePiece));

                    if (bytes_read <= 0) {
                        finished = 1;
                        break;
                    }

                    printf("Team %d Middle Member %d received and forwarding piece:\n",
                           team_id,
                           member_id);
                    print_piece(forward_piece);

                    moved_count++;
                    random_member_delay(config, moved_count, team_id, member_id);

                    write(forward_pipes[member_id][1],
                          &forward_piece,
                          sizeof(FurniturePiece));

                    ReturnMessage response;

                    bytes_read = read(backward_pipes[member_id][0],
                                      &response,
                                      sizeof(ReturnMessage));

                    if (bytes_read <= 0) {
                        finished = 1;
                        break;
                    }

                    printf("Team %d Middle Member %d received response and forwarding back:\n",
                           team_id,
                           member_id);
                    print_return_message(response);

                    moved_count++;
                    random_member_delay(config, moved_count, team_id, member_id);

                    write(backward_pipes[member_id - 1][1],
                          &response,
                          sizeof(ReturnMessage));

                    if (response.status == STATUS_ACCEPTED &&
                        response.piece.piece_id == num_pieces) {
                        finished = 1;
                    }
                }
            }

            exit(0);
        }
    }

    for (int i = 0; i < num_members - 1; i++) {
        close(forward_pipes[i][0]);
        close(forward_pipes[i][1]);
        close(backward_pipes[i][0]);
        close(backward_pipes[i][1]);
    }

    for (int i = 0; i < num_members; i++) {
        wait(NULL);
    }

    printf("Random selection with blocked piece and delay test for Team %d finished.\n",
           team_id);
}
int run_team_round(Config config, int team_id) {
    printf("\n==============================\n");
    printf("Starting round for Team %d\n", team_id);
    printf("==============================\n");

    test_random_selection_with_blocked_piece(config, team_id);

    printf("\nTeam %d completed the round.\n", team_id);

    return 1;
}