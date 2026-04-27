#ifndef CONFIG_H
#define CONFIG_H

typedef struct {
    int team_members;
    int furniture_pieces;
    int max_wins;
    int min_delay;
    int max_delay;
} Config;

Config read_config(const char *filename);
void print_config(Config config);

#endif