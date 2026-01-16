#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "sim.h"

int main(void) {
    int t0, t1, t2, W;
    char infile[256];

    printf("Enter t0 (L0 quantum): ");
    scanf("%d", &t0);
    printf("Enter t1 (L1 quantum): ");
    scanf("%d", &t1);
    printf("Enter t2 (L2 quantum): ");
    scanf("%d", &t2);
    printf("Enter W (aging threshold): ");
    scanf("%d", &W);
    printf("Enter job input file name: ");
    scanf("%255s", infile);

    int trace_int = 0;
    printf("Enable trace? (0/1): ");
    scanf("%d", &trace_int);
    bool trace = (trace_int != 0);

    FILE *f = fopen(infile, "r");
    if (!f) {
        fprintf(stderr, "Failed to open input file: %s\n", infile);
        return 1;
    }

    Sim sim;
    sim_init(&sim, t0, t1, t2, W, trace);
    int n = sim_load_jobs(&sim, f);
    fclose(f);

    if (n <= 0) {
        fprintf(stderr, "No jobs loaded.\n");
        return 1;
    }

    sim_run(&sim);

    double avg_tat, avg_wait, avg_resp;
    sim_metrics(&sim, &avg_tat, &avg_wait, &avg_resp);
    printf("Average Turnaround time: %.2f\n", avg_tat);
    printf("Average Waiting time:    %.2f\n", avg_wait);
    printf("Average Response time:   %.2f\n", avg_resp);

    sim_destroy(&sim);
    return 0;
}
