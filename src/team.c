#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>
#include "team.h"
#include "ipc.h"
#include <time.h>
void test_team_setup(Config config) {
    printf("\nTesting Team Setup:\n");
    printf("Creating team with %d members\n", config.team_members);

    for (int i = 0; i < config.team_members; i++) {
        printf("Member %d ready\n", i);
    }
}

void create_team_processes(Config config, int team_id) {
    printf("\nTesting Fork for Team %d:\n", team_id);

    for (int member_id = 0; member_id < config.team_members; member_id++) {
        pid_t pid = fork();

        if (pid < 0) {
            perror("fork failed");
            exit(1);
        }

        if (pid == 0) {
            printf("Team %d Member %d process started. PID = %d, Parent PID = %d\n",
                   team_id,
                   member_id,
                   getpid(),
                   getppid());

            exit(0);
        }
    }

    for (int i = 0; i < config.team_members; i++) {
        wait(NULL);
    }

    printf("All Team %d member processes finished.\n", team_id);
}
void test_pipe_between_two_members(void) {
    int pipe_fd[2];

    if (pipe(pipe_fd) == -1) {
        perror("pipe failed");
        exit(1);
    }

    pid_t member0 = fork();

    if (member0 < 0) {
        perror("fork failed for member 0");
        exit(1);
    }

    if (member0 == 0) {
        close(pipe_fd[0]);

        FurniturePiece piece;
        piece.piece_id = 1;
        piece.team_id = 1;
        piece.direction = DIRECTION_FORWARD;

        printf("\nMember 0 sending piece:\n");
        print_piece(piece);

        write(pipe_fd[1], &piece, sizeof(FurniturePiece));

        close(pipe_fd[1]);
        exit(0);
    }

    pid_t member1 = fork();

    if (member1 < 0) {
        perror("fork failed for member 1");
        exit(1);
    }

    if (member1 == 0) {
        close(pipe_fd[1]);

        FurniturePiece received_piece;

        read(pipe_fd[0], &received_piece, sizeof(FurniturePiece));

        printf("\nMember 1 received piece:\n");
        print_piece(received_piece);

        close(pipe_fd[0]);
        exit(0);
    }

    close(pipe_fd[0]);
    close(pipe_fd[1]);

    wait(NULL);
    wait(NULL);

    printf("\nPipe test finished.\n");
}
void test_pipe_chain_for_team(Config config, int team_id) {
    int num_members = config.team_members;
    int pipes[num_members - 1][2];

    printf("\nTesting Pipe Chain for Team %d:\n", team_id);

    for (int i = 0; i < num_members - 1; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("pipe failed");
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
            if (member_id == 0) {
                FurniturePiece piece;
                piece.piece_id = 1;
                piece.team_id = team_id;
                piece.direction = DIRECTION_FORWARD;

                printf("Team %d Member %d sending piece:\n", team_id, member_id);
                print_piece(piece);

                write(pipes[0][1], &piece, sizeof(FurniturePiece));
            } else if (member_id == num_members - 1) {
                FurniturePiece received_piece;

                read(pipes[member_id - 1][0], &received_piece, sizeof(FurniturePiece));

                printf("Team %d Member %d received final piece:\n", team_id, member_id);
                print_piece(received_piece);
            } else {
                FurniturePiece received_piece;

                read(pipes[member_id - 1][0], &received_piece, sizeof(FurniturePiece));

                printf("Team %d Member %d received and forwarding piece:\n", team_id, member_id);
                print_piece(received_piece);

                write(pipes[member_id][1], &received_piece, sizeof(FurniturePiece));
            }

            exit(0);
        }
    }

    for (int i = 0; i < num_members - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    for (int i = 0; i < num_members; i++) {
        wait(NULL);
    }

    printf("Pipe chain test for Team %d finished.\n", team_id);
}
void test_sink_checking(Config config, int team_id) {
    int num_members = config.team_members;
    int pipes[num_members - 1][2];
    int expected_piece = 3;

    printf("\nTesting Sink Checking for Team %d:\n", team_id);

    for (int i = 0; i < num_members - 1; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("pipe failed");
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
            if (member_id == 0) {
                FurniturePiece piece;
                piece.piece_id = 1;
                piece.team_id = team_id;
                piece.direction = DIRECTION_FORWARD;

                printf("Team %d Member %d sending piece:\n", team_id, member_id);
                print_piece(piece);

                write(pipes[0][1], &piece, sizeof(FurniturePiece));
            } else if (member_id == num_members - 1) {
                FurniturePiece received_piece;

                read(pipes[member_id - 1][0], &received_piece, sizeof(FurniturePiece));

                printf("Team %d Sink Member %d received piece:\n", team_id, member_id);
                print_piece(received_piece);

                if (received_piece.piece_id == expected_piece) {
                    printf("Result: ACCEPTED  Piece %d is the expected piece.\n",
                           received_piece.piece_id);
                } else {
                    printf("Result: WRONG  Expected piece %d but received piece %d.\n",
                           expected_piece,
                           received_piece.piece_id);
                }
            } else {
                FurniturePiece received_piece;

                read(pipes[member_id - 1][0], &received_piece, sizeof(FurniturePiece));

                printf("Team %d Member %d received and forwarding piece:\n", team_id, member_id);
                print_piece(received_piece);

                write(pipes[member_id][1], &received_piece, sizeof(FurniturePiece));
            }

            exit(0);
        }
    }

    for (int i = 0; i < num_members - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    for (int i = 0; i < num_members; i++) {
        wait(NULL);
    }

    printf("Sink checking test for Team %d finished.\n", team_id);
} 
void test_backward_return(Config config, int team_id) {
    int num_members = config.team_members;
    int forward_pipes[num_members - 1][2];
    int backward_pipes[num_members - 1][2];
    int expected_piece = 1;

    printf("\nTesting Backward Return for Team %d:\n", team_id);

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
            if (member_id == 0) {
                FurniturePiece piece;

                piece.piece_id = 3;   // intentionally wrong
                piece.team_id = team_id;
                piece.direction = DIRECTION_FORWARD;

                printf("Team %d Source Member %d sending wrong piece:\n", team_id, member_id);
                print_piece(piece);

                write(forward_pipes[0][1], &piece, sizeof(FurniturePiece));

                FurniturePiece returned_piece;
                read(backward_pipes[0][0], &returned_piece, sizeof(FurniturePiece));

                printf("Team %d Source Member %d received returned piece:\n", team_id, member_id);
                print_piece(returned_piece);

            } else if (member_id == num_members - 1) {
                FurniturePiece received_piece;

                read(forward_pipes[member_id - 1][0], &received_piece, sizeof(FurniturePiece));

                printf("Team %d Sink Member %d received piece:\n", team_id, member_id);
                print_piece(received_piece);

                if (received_piece.piece_id == expected_piece) {
                    printf("Result: ACCEPTED. Piece %d is the expected piece.\n",
                           received_piece.piece_id);
                } else {
                    printf("Result: WRONG. Expected piece %d but received piece %d.\n",
                           expected_piece,
                           received_piece.piece_id);

                    received_piece.direction = DIRECTION_BACKWARD;

                    printf("Team %d Sink Member %d returning piece backward:\n", team_id, member_id);
                    print_piece(received_piece);

                    write(backward_pipes[member_id - 1][1],
                          &received_piece,
                          sizeof(FurniturePiece));
                }

            } else {
                FurniturePiece forward_piece;

                read(forward_pipes[member_id - 1][0],
                     &forward_piece,
                     sizeof(FurniturePiece));

                printf("Team %d Middle Member %d received forward piece:\n", team_id, member_id);
                print_piece(forward_piece);

                write(forward_pipes[member_id][1],
                      &forward_piece,
                      sizeof(FurniturePiece));

                FurniturePiece backward_piece;

                read(backward_pipes[member_id][0],
                     &backward_piece,
                     sizeof(FurniturePiece));

                printf("Team %d Middle Member %d received backward piece and forwarding back:\n",
                       team_id,
                       member_id);
                print_piece(backward_piece);

                write(backward_pipes[member_id - 1][1],
                      &backward_piece,
                      sizeof(FurniturePiece));
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

    printf("Backward return test for Team %d finished.\n", team_id);
}
void test_wrong_then_correct(Config config, int team_id) {
    int num_members = config.team_members;
    int forward_pipes[num_members - 1][2];
    int backward_pipes[num_members - 1][2];
    int expected_piece = 1;

    printf("\nTesting Wrong Then Correct for Team %d:\n", team_id);

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
            if (member_id == 0) {
                FurniturePiece wrong_piece;
                wrong_piece.piece_id = 3;
                wrong_piece.team_id = team_id;
                wrong_piece.direction = DIRECTION_FORWARD;

                printf("Team %d Source Member %d sending wrong piece:\n", team_id, member_id);
                print_piece(wrong_piece);

                write(forward_pipes[0][1], &wrong_piece, sizeof(FurniturePiece));

                FurniturePiece returned_piece;
                read(backward_pipes[0][0], &returned_piece, sizeof(FurniturePiece));

                printf("Team %d Source Member %d received returned wrong piece:\n", team_id, member_id);
                print_piece(returned_piece);

                FurniturePiece correct_piece;
                correct_piece.piece_id = 1;
                correct_piece.team_id = team_id;
                correct_piece.direction = DIRECTION_FORWARD;

                printf("Team %d Source Member %d now sending correct piece:\n", team_id, member_id);
                print_piece(correct_piece);

                write(forward_pipes[0][1], &correct_piece, sizeof(FurniturePiece));

            } else if (member_id == num_members - 1) {
                for (int attempt = 0; attempt < 2; attempt++) {
                    FurniturePiece received_piece;

                    read(forward_pipes[member_id - 1][0],
                         &received_piece,
                         sizeof(FurniturePiece));

                    printf("Team %d Sink Member %d received piece:\n", team_id, member_id);
                    print_piece(received_piece);

                    if (received_piece.piece_id == expected_piece) {
                        printf("Result: ACCEPTED. Piece %d is the expected piece.\n",
                               received_piece.piece_id);
                    } else {
                        printf("Result: WRONG. Expected piece %d but received piece %d.\n",
                               expected_piece,
                               received_piece.piece_id);

                        received_piece.direction = DIRECTION_BACKWARD;

                        printf("Team %d Sink Member %d returning piece backward:\n", team_id, member_id);
                        print_piece(received_piece);

                        write(backward_pipes[member_id - 1][1],
                              &received_piece,
                              sizeof(FurniturePiece));
                    }
                }

            } else {
                for (int attempt = 0; attempt < 2; attempt++) {
                    FurniturePiece forward_piece;

                    read(forward_pipes[member_id - 1][0],
                         &forward_piece,
                         sizeof(FurniturePiece));

                    printf("Team %d Middle Member %d received forward piece:\n",
                           team_id,
                           member_id);
                    print_piece(forward_piece);

                    write(forward_pipes[member_id][1],
                          &forward_piece,
                          sizeof(FurniturePiece));

                    if (forward_piece.piece_id != expected_piece) {
                        FurniturePiece backward_piece;

                        read(backward_pipes[member_id][0],
                             &backward_piece,
                             sizeof(FurniturePiece));

                        printf("Team %d Middle Member %d received backward piece and forwarding back:\n",
                               team_id,
                               member_id);
                        print_piece(backward_piece);

                        write(backward_pipes[member_id - 1][1],
                              &backward_piece,
                              sizeof(FurniturePiece));
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

    printf("Wrong then correct test for Team %d finished.\n", team_id);
}
void test_source_piece_selection(Config config, int team_id) {
    int num_pieces = config.furniture_pieces;

    printf("\nTesting Source Piece Selection for Team %d:\n", team_id);
    printf("Available furniture pieces:\n");

    int pieces[num_pieces];

    for (int i = 0; i < num_pieces; i++) {
        pieces[i] = i + 1;
        printf("Piece %d available\n", pieces[i]);
    }

    printf("\nSource member will select pieces for testing:\n");

    int selected_piece_1 = 3;
    int selected_piece_2 = 1;

    printf("First selected piece: %d\n", selected_piece_1);
    printf("Second selected piece: %d\n", selected_piece_2);

    if (selected_piece_1 != 1) {
        printf("Piece %d is not expected first, so it would be returned.\n",
               selected_piece_1);
    }

    if (selected_piece_2 == 1) {
        printf("Piece %d is expected first, so it would be accepted.\n",
               selected_piece_2);
    }

    printf("Source piece selection test finished.\n");
}
void test_selection_with_pipes(Config config, int team_id) {
    int num_members = config.team_members;
    int num_pieces = config.furniture_pieces;
    int expected_piece = 1;

    int forward_pipes[num_members - 1][2];
    int backward_pipes[num_members - 1][2];

    printf("\nTesting Source Selection With Pipes for Team %d:\n", team_id);

    int pieces[num_pieces];

    for (int i = 0; i < num_pieces; i++) {
        pieces[i] = i + 1;
    }

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
            if (member_id == 0) {
                int selected_indices[2] = {2, 0};

                for (int attempt = 0; attempt < 2; attempt++) {
                    int selected_piece_id = pieces[selected_indices[attempt]];

                    FurniturePiece piece;
                    piece.piece_id = selected_piece_id;
                    piece.team_id = team_id;
                    piece.direction = DIRECTION_FORWARD;

                    printf("Team %d Source Member %d selected and sending piece:\n",
                           team_id,
                           member_id);
                    print_piece(piece);

                    write(forward_pipes[0][1],
                          &piece,
                          sizeof(FurniturePiece));

                    if (piece.piece_id != expected_piece) {
                        FurniturePiece returned_piece;

                        read(backward_pipes[0][0],
                             &returned_piece,
                             sizeof(FurniturePiece));

                        printf("Team %d Source Member %d received returned piece:\n",
                               team_id,
                               member_id);
                        print_piece(returned_piece);
                    }
                }

            } else if (member_id == num_members - 1) {
                for (int attempt = 0; attempt < 2; attempt++) {
                    FurniturePiece received_piece;

                    read(forward_pipes[member_id - 1][0],
                         &received_piece,
                         sizeof(FurniturePiece));

                    printf("Team %d Sink Member %d received piece:\n",
                           team_id,
                           member_id);
                    print_piece(received_piece);

                    if (received_piece.piece_id == expected_piece) {
                        printf("Result: ACCEPTED. Piece %d is the expected piece.\n",
                               received_piece.piece_id);
                    } else {
                        printf("Result: WRONG. Expected piece %d but received piece %d.\n",
                               expected_piece,
                               received_piece.piece_id);

                        received_piece.direction = DIRECTION_BACKWARD;

                        printf("Team %d Sink Member %d returning piece backward:\n",
                               team_id,
                               member_id);
                        print_piece(received_piece);

                        write(backward_pipes[member_id - 1][1],
                              &received_piece,
                              sizeof(FurniturePiece));
                    }
                }

            } else {
                for (int attempt = 0; attempt < 2; attempt++) {
                    FurniturePiece forward_piece;

                    read(forward_pipes[member_id - 1][0],
                         &forward_piece,
                         sizeof(FurniturePiece));

                    printf("Team %d Middle Member %d received and forwarding piece:\n",
                           team_id,
                           member_id);
                    print_piece(forward_piece);

                    write(forward_pipes[member_id][1],
                          &forward_piece,
                          sizeof(FurniturePiece));

                    if (forward_piece.piece_id != expected_piece) {
                        FurniturePiece backward_piece;

                        read(backward_pipes[member_id][0],
                             &backward_piece,
                             sizeof(FurniturePiece));

                        printf("Team %d Middle Member %d received backward piece and forwarding back:\n",
                               team_id,
                               member_id);
                        print_piece(backward_piece);

                        write(backward_pipes[member_id - 1][1],
                              &backward_piece,
                              sizeof(FurniturePiece));
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

    printf("Source selection with pipes test for Team %d finished.\n", team_id);
}
void test_status_pipe(Config config, int team_id) {
    int num_members = config.team_members;
    int expected_piece = 1;

    int forward_pipes[num_members - 1][2];
    int backward_pipes[num_members - 1][2];
    int status_pipe[2];

    printf("\nTesting Status Pipe for Team %d:\n", team_id);

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

    if (pipe(status_pipe) == -1) {
        perror("status pipe failed");
        exit(1);
    }

    for (int member_id = 0; member_id < num_members; member_id++) {
        pid_t pid = fork();

        if (pid < 0) {
            perror("fork failed");
            exit(1);
        }

        if (pid == 0) {
            if (member_id == 0) {
                int selected_pieces[2] = {3, 1};

                for (int attempt = 0; attempt < 2; attempt++) {
                    FurniturePiece piece;
                    piece.piece_id = selected_pieces[attempt];
                    piece.team_id = team_id;
                    piece.direction = DIRECTION_FORWARD;

                    printf("Team %d Source Member %d sending piece:\n",
                           team_id,
                           member_id);
                    print_piece(piece);

                    write(forward_pipes[0][1],
                          &piece,
                          sizeof(FurniturePiece));

                    if (piece.piece_id != expected_piece) {
                        FurniturePiece returned_piece;

                        read(backward_pipes[0][0],
                             &returned_piece,
                             sizeof(FurniturePiece));

                        printf("Team %d Source Member %d received returned piece:\n",
                               team_id,
                               member_id);
                        print_piece(returned_piece);
                    } else {
                        int status;

                        read(status_pipe[0],
                             &status,
                             sizeof(int));

                        if (status == STATUS_ACCEPTED) {
                            printf("Team %d Source Member %d received ACCEPTED status for piece %d.\n",
                                   team_id,
                                   member_id,
                                   piece.piece_id);
                        }
                    }
                }

            } else if (member_id == num_members - 1) {
                for (int attempt = 0; attempt < 2; attempt++) {
                    FurniturePiece received_piece;

                    read(forward_pipes[member_id - 1][0],
                         &received_piece,
                         sizeof(FurniturePiece));

                    printf("Team %d Sink Member %d received piece:\n",
                           team_id,
                           member_id);
                    print_piece(received_piece);

                    if (received_piece.piece_id == expected_piece) {
                        int status = STATUS_ACCEPTED;

                        printf("Result: ACCEPTED. Piece %d is the expected piece.\n",
                               received_piece.piece_id);

                        write(status_pipe[1],
                              &status,
                              sizeof(int));
                    } else {
                        printf("Result: WRONG. Expected piece %d but received piece %d.\n",
                               expected_piece,
                               received_piece.piece_id);

                        received_piece.direction = DIRECTION_BACKWARD;

                        printf("Team %d Sink Member %d returning piece backward:\n",
                               team_id,
                               member_id);
                        print_piece(received_piece);

                        write(backward_pipes[member_id - 1][1],
                              &received_piece,
                              sizeof(FurniturePiece));
                    }
                }

            } else {
                for (int attempt = 0; attempt < 2; attempt++) {
                    FurniturePiece forward_piece;

                    read(forward_pipes[member_id - 1][0],
                         &forward_piece,
                         sizeof(FurniturePiece));

                    printf("Team %d Middle Member %d received and forwarding piece:\n",
                           team_id,
                           member_id);
                    print_piece(forward_piece);

                    write(forward_pipes[member_id][1],
                          &forward_piece,
                          sizeof(FurniturePiece));

                    if (forward_piece.piece_id != expected_piece) {
                        FurniturePiece backward_piece;

                        read(backward_pipes[member_id][0],
                             &backward_piece,
                             sizeof(FurniturePiece));

                        printf("Team %d Middle Member %d received backward piece and forwarding back:\n",
                               team_id,
                               member_id);
                        print_piece(backward_piece);

                        write(backward_pipes[member_id - 1][1],
                              &backward_piece,
                              sizeof(FurniturePiece));
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

    close(status_pipe[0]);
    close(status_pipe[1]);

    for (int i = 0; i < num_members; i++) {
        wait(NULL);
    }

    printf("Status pipe test for Team %d finished.\n", team_id);
}
void test_loop_until_accepted(Config config, int team_id) {
    int num_members = config.team_members;
    int expected_piece = 1;

    int forward_pipes[num_members - 1][2];
    int backward_pipes[num_members - 1][2];
    int status_pipe[2];

    printf("\nTesting Loop Until Accepted for Team %d:\n", team_id);

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

    if (pipe(status_pipe) == -1) {
        perror("status pipe failed");
        exit(1);
    }

    for (int member_id = 0; member_id < num_members; member_id++) {
        pid_t pid = fork();

        if (pid < 0) {
            perror("fork failed");
            exit(1);
        }

        if (pid == 0) {
            if (member_id == 0) {
                int candidate_pieces[] = {3, 4, 1};
                int num_candidates = 3;
                int accepted = 0;

                for (int attempt = 0; attempt < num_candidates && !accepted; attempt++) {
                    FurniturePiece piece;
                    piece.piece_id = candidate_pieces[attempt];
                    piece.team_id = team_id;
                    piece.direction = DIRECTION_FORWARD;

                    printf("Team %d Source Member %d attempt %d sending piece:\n",
                           team_id,
                           member_id,
                           attempt + 1);
                    print_piece(piece);

                    write(forward_pipes[0][1],
                          &piece,
                          sizeof(FurniturePiece));

                    if (piece.piece_id != expected_piece) {
                        FurniturePiece returned_piece;

                        read(backward_pipes[0][0],
                             &returned_piece,
                             sizeof(FurniturePiece));

                        printf("Team %d Source Member %d received returned wrong piece:\n",
                               team_id,
                               member_id);
                        print_piece(returned_piece);
                    } else {
                        int status;

                        read(status_pipe[0],
                             &status,
                             sizeof(int));

                        if (status == STATUS_ACCEPTED) {
                            printf("Team %d Source Member %d knows piece %d was accepted.\n",
                                   team_id,
                                   member_id,
                                   piece.piece_id);
                            accepted = 1;
                        }
                    }
                }

            } else if (member_id == num_members - 1) {
                int accepted = 0;

                while (!accepted) {
                    FurniturePiece received_piece;

                    read(forward_pipes[member_id - 1][0],
                         &received_piece,
                         sizeof(FurniturePiece));

                    printf("Team %d Sink Member %d received piece:\n",
                           team_id,
                           member_id);
                    print_piece(received_piece);

                    if (received_piece.piece_id == expected_piece) {
                        int status = STATUS_ACCEPTED;

                        printf("Result: ACCEPTED. Piece %d is the expected piece.\n",
                               received_piece.piece_id);

                        write(status_pipe[1],
                              &status,
                              sizeof(int));

                        accepted = 1;
                    } else {
                        printf("Result: WRONG. Expected piece %d but received piece %d.\n",
                               expected_piece,
                               received_piece.piece_id);

                        received_piece.direction = DIRECTION_BACKWARD;

                        printf("Team %d Sink Member %d returning piece backward:\n",
                               team_id,
                               member_id);
                        print_piece(received_piece);

                        write(backward_pipes[member_id - 1][1],
                              &received_piece,
                              sizeof(FurniturePiece));
                    }
                }

            } else {
                int accepted = 0;

                while (!accepted) {
                    FurniturePiece forward_piece;

                    read(forward_pipes[member_id - 1][0],
                         &forward_piece,
                         sizeof(FurniturePiece));

                    printf("Team %d Middle Member %d received and forwarding piece:\n",
                           team_id,
                           member_id);
                    print_piece(forward_piece);

                    write(forward_pipes[member_id][1],
                          &forward_piece,
                          sizeof(FurniturePiece));

                    if (forward_piece.piece_id != expected_piece) {
                        FurniturePiece backward_piece;

                        read(backward_pipes[member_id][0],
                             &backward_piece,
                             sizeof(FurniturePiece));

                        printf("Team %d Middle Member %d received backward piece and forwarding back:\n",
                               team_id,
                               member_id);
                        print_piece(backward_piece);

                        write(backward_pipes[member_id - 1][1],
                              &backward_piece,
                              sizeof(FurniturePiece));
                    } else {
                        accepted = 1;
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

    close(status_pipe[0]);
    close(status_pipe[1]);

    for (int i = 0; i < num_members; i++) {
        wait(NULL);
    }

    printf("Loop until accepted test for Team %d finished.\n", team_id);
}
void test_return_message_protocol(Config config, int team_id) {
    int num_members = config.team_members;
    int expected_piece = 1;

    int forward_pipes[num_members - 1][2];
    int backward_pipes[num_members - 1][2];

    printf("\nTesting ReturnMessage Protocol for Team %d:\n", team_id);

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
            if (member_id == 0) {
                int selected_pieces[2] = {3, 1};

                for (int attempt = 0; attempt < 2; attempt++) {
                    FurniturePiece piece;
                    piece.piece_id = selected_pieces[attempt];
                    piece.team_id = team_id;
                    piece.direction = DIRECTION_FORWARD;

                    printf("Team %d Source Member %d sending piece:\n",
                           team_id,
                           member_id);
                    print_piece(piece);

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
                        printf("Team %d Source knows piece %d was accepted.\n",
                               team_id,
                               response.piece.piece_id);
                    } else if (response.status == STATUS_WRONG) {
                        printf("Team %d Source knows piece %d was wrong and returned.\n",
                               team_id,
                               response.piece.piece_id);
                    }
                }

            } else if (member_id == num_members - 1) {
                for (int attempt = 0; attempt < 2; attempt++) {
                    FurniturePiece received_piece;

                    read(forward_pipes[member_id - 1][0],
                         &received_piece,
                         sizeof(FurniturePiece));

                    printf("Team %d Sink Member %d received piece:\n",
                           team_id,
                           member_id);
                    print_piece(received_piece);

                    ReturnMessage response;
                    response.piece = received_piece;

                    if (received_piece.piece_id == expected_piece) {
                        response.status = STATUS_ACCEPTED;

                        printf("Result: ACCEPTED. Piece %d is the expected piece.\n",
                               received_piece.piece_id);
                    } else {
                        response.status = STATUS_WRONG;
                        response.piece.direction = DIRECTION_BACKWARD;

                        printf("Result: WRONG. Expected piece %d but received piece %d.\n",
                               expected_piece,
                               received_piece.piece_id);
                    }

                    printf("Team %d Sink Member %d sending response backward:\n",
                           team_id,
                           member_id);
                    print_return_message(response);

                    write(backward_pipes[member_id - 1][1],
                          &response,
                          sizeof(ReturnMessage));
                }

            } else {
                for (int attempt = 0; attempt < 2; attempt++) {
                    FurniturePiece forward_piece;

                    read(forward_pipes[member_id - 1][0],
                         &forward_piece,
                         sizeof(FurniturePiece));

                    printf("Team %d Middle Member %d received and forwarding piece:\n",
                           team_id,
                           member_id);
                    print_piece(forward_piece);

                    write(forward_pipes[member_id][1],
                          &forward_piece,
                          sizeof(FurniturePiece));

                    ReturnMessage response;

                    read(backward_pipes[member_id][0],
                         &response,
                         sizeof(ReturnMessage));

                    printf("Team %d Middle Member %d received response and forwarding back:\n",
                           team_id,
                           member_id);
                    print_return_message(response);

                    write(backward_pipes[member_id - 1][1],
                          &response,
                          sizeof(ReturnMessage));
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

    printf("ReturnMessage protocol test for Team %d finished.\n", team_id);
}
void test_random_selection_with_return_message(Config config, int team_id) {
    int num_members = config.team_members;
    int num_pieces = config.furniture_pieces;

    int forward_pipes[num_members - 1][2];
    int backward_pipes[num_members - 1][2];

    printf("\nTesting Random Selection With ReturnMessage for Team %d:\n", team_id);

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
            if (member_id == 0) {
                int accepted_count = 0;
                int available[num_pieces];

                srand(getpid());

                for (int i = 0; i < num_pieces; i++) {
                    available[i] = 1;
                }

                while (accepted_count < num_pieces) {
                    int selected_index;
                    int selected_piece_id;

                    do {
                        selected_index = rand() % num_pieces;
                        selected_piece_id = selected_index + 1;
                    } while (available[selected_index] == 0);

                    FurniturePiece piece;
                    piece.piece_id = selected_piece_id;
                    piece.team_id = team_id;
                    piece.direction = DIRECTION_FORWARD;

                    printf("Team %d Source Member %d randomly selected piece:\n",
                           team_id,
                           member_id);
                    print_piece(piece);

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

                        printf("Team %d Source accepted count = %d/%d\n",
                               team_id,
                               accepted_count,
                               num_pieces);
                    } else if (response.status == STATUS_WRONG) {
                        printf("Team %d Source will keep piece %d available for later.\n",
                               team_id,
                               response.piece.piece_id);
                    }
                }

                printf("Team %d finished all furniture pieces using random selection.\n",
                       team_id);

            } else if (member_id == num_members - 1) {
                int expected_piece = 1;

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

                        expected_piece++;
                    } else {
                        response.status = STATUS_WRONG;
                        response.piece.direction = DIRECTION_BACKWARD;

                        printf("Result: WRONG. Expected piece %d but received piece %d.\n",
                               expected_piece,
                               received_piece.piece_id);
                    }

                    printf("Team %d Sink Member %d sending response backward:\n",
                           team_id,
                           member_id);
                    print_return_message(response);

                    write(backward_pipes[member_id - 1][1],
                          &response,
                          sizeof(ReturnMessage));
                }

            } else {
                int finished = 0;

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

    printf("Random selection with ReturnMessage test for Team %d finished.\n",
           team_id);
}
void test_random_selection_with_blocked_piece1(Config config, int team_id) {
    int num_members = config.team_members;
    int num_pieces = config.furniture_pieces;

    int forward_pipes[num_members - 1][2];
    int backward_pipes[num_members - 1][2];

    printf("\nTesting Random Selection With Blocked Piece for Team %d:\n", team_id);

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
            if (member_id == 0) {
                int accepted_count = 0;
                int available[num_pieces];
                int blocked_piece = -1;

                srand(getpid());

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

                        expected_piece++;
                    } else {
                        response.status = STATUS_WRONG;
                        response.piece.direction = DIRECTION_BACKWARD;

                        printf("Result: WRONG. Expected piece %d but received piece %d.\n",
                               expected_piece,
                               received_piece.piece_id);
                    }

                    printf("Team %d Sink Member %d sending response backward:\n",
                           team_id,
                           member_id);
                    print_return_message(response);

                    write(backward_pipes[member_id - 1][1],
                          &response,
                          sizeof(ReturnMessage));
                }

            } else {
                int finished = 0;

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

    printf("Random selection with blocked piece test for Team %d finished.\n",
           team_id);
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
            srand(getpid() + time(NULL));

            if (member_id == 0) {
                int accepted_count = 0;
                int available[num_pieces];
                int blocked_piece = -1;
                int moved_count = 0;

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

                        expected_piece++;
                    } else {
                        response.status = STATUS_WRONG;
                        response.piece.direction = DIRECTION_BACKWARD;

                        printf("Result: WRONG. Expected piece %d but received piece %d.\n",
                               expected_piece,
                               received_piece.piece_id);
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