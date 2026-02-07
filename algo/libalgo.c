#include <stdio.h>
typedef struct {
    // 0 = 12, 1 = 11, 2 = 10, 3 = 9, 4 = 8, 5 = 7

    int dr;
    float bw;
    int pwr;
} State;

State pid(State *current) {
    current->dr = 5;
    // Placeholder
    printf("Current State: DR %d, BW %.2f, PWR %d\n", current->dr, current->bw, current->pwr);
    return *current;
}