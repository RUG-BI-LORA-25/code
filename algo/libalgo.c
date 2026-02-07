#include <stdio.h>
typedef struct {
    // 0 = 12, 1 = 11, 2 = 10, 3 = 9, 4 = 8, 5 = 7

    int dr;
    float bw;
    int pwr;
} State;

State pid(State *current) {
    static int counter = 0;
    printf("PID called. Counter: %d\n", counter);
    current->dr = counter++;
    counter = counter % 6; 
    // Placeholder
    printf("Current State: DR %d, BW %.2f, PWR %d\n", current->dr, current->bw, current->pwr);
    return *current;
}