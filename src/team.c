#include <stdio.h>
#include "team.h"

void test_team_setup(Config config) {
    printf("\nTesting Team Setup:\n");
    printf("Creating team with %d members\n", config.team_members);

    for (int i = 0; i < config.team_members; i++) {
        printf("Member %d ready\n", i);
    }
}