#include <stdio.h>
#include <GL/glut.h>
#include "graphics.h"

static int g_team1_wins = 0;
static int g_team2_wins = 0;
static int g_winning_team = 0;

static void draw_text(float x, float y, const char *text) {
    glRasterPos2f(x, y);

    for (int i = 0; text[i] != '\0'; i++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, text[i]);
    }
}

static void display(void) {
    char buffer[128];

    glClear(GL_COLOR_BUFFER_BIT);

    glColor3f(1.0f, 1.0f, 1.0f);

    draw_text(-0.65f, 0.75f, "Furniture Moving Competition");

    snprintf(buffer, sizeof(buffer), "Competition Winner: Team %d", g_winning_team);
    draw_text(-0.55f, 0.45f, buffer);

    snprintf(buffer, sizeof(buffer), "Team 1 Wins: %d", g_team1_wins);
    draw_text(-0.55f, 0.20f, buffer);

    snprintf(buffer, sizeof(buffer), "Team 2 Wins: %d", g_team2_wins);
    draw_text(-0.55f, 0.00f, buffer);

    draw_text(-0.75f, -0.35f, "Source  ->  Middle Members  ->  Sink / House");
    draw_text(-0.75f, -0.55f, "IPC Used: Pipes, Signals, FIFO, OpenMP");

    glFlush();
}

void show_final_result_graphics(int team1_wins, int team2_wins, int winning_team) {
    int argc = 1;
    char *argv[] = {"project1", NULL};

    g_team1_wins = team1_wins;
    g_team2_wins = team2_wins;
    g_winning_team = winning_team;

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
    glutInitWindowSize(700, 450);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("Project 1 Final Result");

    glClearColor(0.05f, 0.05f, 0.08f, 1.0f);

    glutDisplayFunc(display);
    glutMainLoop();
}