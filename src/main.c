#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#include "config.h"
#include "team.h"
#include "ipc.h"

static volatile sig_atomic_t start_signal_received = 0;

void handle_start_signal(int signal_number) {
    (void) signal_number;
    start_signal_received = 1;
}

void wait_for_start_signal(void) {
    struct sigaction action;

    action.sa_handler = handle_start_signal;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;

    if (sigaction(SIGUSR1, &action, NULL) == -1) {
        perror("sigaction failed");
        exit(1);
    }

    while (!start_signal_received) {
        pause();
    }
}

void start_logger_process(void) {
    int fd = open(FIFO_PATH, O_RDWR);

    if (fd == -1) {
        perror("logger failed to open FIFO");
        exit(1);
    }

    char buffer[256];

    while (1) {
        ssize_t bytes_read = read(fd, buffer, sizeof(buffer) - 1);

        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';

            if (strstr(buffer, "STOP_LOGGER") != NULL) {
                break;
            }

            printf("[FIFO LOG] %s", buffer);
            fflush(stdout);
        }
    }

    close(fd);
    exit(0);
}

int run_parallel_round(Config config, int round_number) {
    printf("\n====================================\n");
    printf("Starting Round %d: Team 1 vs Team 2\n", round_number);
    printf("====================================\n");

    pid_t team1_pid = fork();

    if (team1_pid < 0) {
        perror("fork failed for Team 1");
        exit(1);
    }

    if (team1_pid == 0) {
        setpgid(0, 0);

        start_signal_received = 0;

        printf("Team 1 is ready and waiting for start signal...\n");
        wait_for_start_signal();

        printf("Team 1 received start signal. Starting round now.\n");

        run_team_round(config, 1);

        exit(1);
    }

    setpgid(team1_pid, team1_pid);

    pid_t team2_pid = fork();

    if (team2_pid < 0) {
        perror("fork failed for Team 2");
        exit(1);
    }

    if (team2_pid == 0) {
        setpgid(0, 0);

        start_signal_received = 0;

        printf("Team 2 is ready and waiting for start signal...\n");
        wait_for_start_signal();

        printf("Team 2 received start signal. Starting round now.\n");

        run_team_round(config, 2);

        exit(2);
    }

    setpgid(team2_pid, team2_pid);

    /*
     * Give both child processes a moment to install their SIGUSR1 handlers
     * and enter the waiting state.
     */
    sleep(1);

    printf("\nParent sending SIGUSR1 start signal to both teams...\n");

    if (kill(-team1_pid, SIGUSR1) == -1) {
        perror("failed to send start signal to Team 1");
    }

    if (kill(-team2_pid, SIGUSR1) == -1) {
        perror("failed to send start signal to Team 2");
    }

    int status;
    pid_t finished_pid = wait(&status);

    int winning_team = -1;
    pid_t losing_pid = -1;

    if (finished_pid == team1_pid) {
        winning_team = 1;
        losing_pid = team2_pid;
    } else if (finished_pid == team2_pid) {
        winning_team = 2;
        losing_pid = team1_pid;
    }

    printf("\n==============================\n");

    if (WIFEXITED(status)) {
        printf("Round %d winner: Team %d\n", round_number, winning_team);

        char log_message[128];
        snprintf(log_message, sizeof(log_message),
                 "[Round %d] Team %d won the round.",
                 round_number,
                 winning_team);
        log_event(log_message);
    } else {
        printf("Round %d ended unexpectedly.\n", round_number);
    }

    printf("==============================\n");

    if (losing_pid > 0) {
        printf("Stopping losing team using SIGTERM...\n");

        if (kill(-losing_pid, SIGTERM) == -1) {
            if (errno == ESRCH) {
                printf("Losing team already finished.\n");
            } else {
                perror("failed to stop losing team");
            }
        }

        waitpid(losing_pid, NULL, 0);
    }

    printf("Round %d finished.\n", round_number);

    return winning_team;
}

int main(int argc, char *argv[]) {
    const char *config_file = "config.txt";

    if (argc > 1) {
        config_file = argv[1];
    }

    Config config = read_config(config_file);
    print_config(config);

    unlink(FIFO_PATH);

    if (mkfifo(FIFO_PATH, 0666) == -1) {
        perror("mkfifo failed");
        exit(1);
    }

    pid_t logger_pid = fork();

    if (logger_pid < 0) {
        perror("fork failed for logger");
        exit(1);
    }

    if (logger_pid == 0) {
        start_logger_process();
    }

    int team1_wins = 0;
    int team2_wins = 0;
    int round_number = 1;

    printf("\n====================================\n");
    printf("Starting Competition\n");
    printf("First team to reach %d wins is the champion.\n", config.max_wins);
    printf("====================================\n");

    while (team1_wins < config.max_wins &&
           team2_wins < config.max_wins) {

        int winner = run_parallel_round(config, round_number);

        if (winner == 1) {
            team1_wins++;
        } else if (winner == 2) {
            team2_wins++;
        } else {
            printf("No valid winner for round %d.\n", round_number);
            break;
        }

        printf("\nCurrent Score:\n");
        printf("Team 1: %d wins\n", team1_wins);
        printf("Team 2: %d wins\n", team2_wins);

        round_number++;
    }

    printf("\n====================================\n");

    if (team1_wins == config.max_wins) {
        printf("Competition Winner: Team 1\n");
    } else if (team2_wins == config.max_wins) {
        printf("Competition Winner: Team 2\n");
    } else {
        printf("Competition ended without a final winner.\n");
    }

    printf("Final Score:\n");
    printf("Team 1: %d wins\n", team1_wins);
    printf("Team 2: %d wins\n", team2_wins);
    printf("====================================\n");

    log_event("STOP_LOGGER");
    waitpid(logger_pid, NULL, 0);
    unlink(FIFO_PATH);

    return 0;
}