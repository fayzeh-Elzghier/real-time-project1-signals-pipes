#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"

Config read_config(const char *filename) {
    Config config;

    config.team_members = 3;
    config.furniture_pieces = 5;
    config.max_wins = 2;
    config.min_delay = 1;
    config.max_delay = 2;

    FILE *file = fopen(filename, "r");

    if (file == NULL) {
        printf("Error: Could not open config file %s\n", filename);
        printf("Using default values.\n");
        return config;
    }

    char line[100];
    char key[50];
    int value;

    while (fgets(line, sizeof(line), file)) {
        if (sscanf(line, "%49[^=]=%d", key, &value) == 2) {
            if (strcmp(key, "TEAM_MEMBERS") == 0) {
                config.team_members = value;
            } else if (strcmp(key, "FURNITURE_PIECES") == 0) {
                config.furniture_pieces = value;
            } else if (strcmp(key, "MAX_WINS") == 0) {
                config.max_wins = value;
            } else if (strcmp(key, "MIN_DELAY") == 0) {
                config.min_delay = value;
            } else if (strcmp(key, "MAX_DELAY") == 0) {
                config.max_delay = value;
            }
        }
    }

    fclose(file);
    return config;
}

void print_config(Config config) {
    printf("Configuration Loaded:\n");
    printf("Team members: %d\n", config.team_members);
    printf("Furniture pieces: %d\n", config.furniture_pieces);
    printf("Max wins: %d\n", config.max_wins);
    printf("Min delay: %d\n", config.min_delay);
    printf("Max delay: %d\n", config.max_delay);
}