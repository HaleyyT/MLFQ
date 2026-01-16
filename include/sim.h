#ifndef SIM_H
#define SIM_H
#include <stdio.h>
#include <sys/types.h>   // for pid_t
#include <stdbool.h>
#include <stddef.h>

typedef struct Job {
    int id;
    int arrival;
    int cpu_total;
    int cpu_remaining;

    int init_prio;     // 0,1,2 at arrival
    int curr_level;    // 0,1,2 while running/queued
    int time_used_in_quantum;

    int first_start_time; // -1 until first dispatch
    int finish_time;      // set when done

    int last_enqueue_time; // bookkeeping for waiting/aging
    bool has_started;
    pid_t pid;   

} Job;

typedef struct Q Q;   // opaque queue (of Job*)

typedef struct Sim {
    Q* dispatch;
    Q* l0;
    Q* l1;
    Q* l2;

    Job* running;

    int t;             // global time (ticks)
    int t0, t1, t2;    // quanta
    int W;             // aging threshold
    bool trace;

    // metrics accumulation
    size_t job_count;

    // aging bookkeeping
    int l1_front_since;   // time when current L1 head became front, or -1 if empty
    int l2_front_since;   // time when current L2 head became front, or -1 if empty
    int l1_front_id;      // job id of current L1 head, or -1
    int l2_front_id;      // job id of current L2 head, or -1


    double total_tat, total_wait, total_resp;

} Sim;

/* Lifecycle */
void sim_init(Sim* s, int t0, int t1, int t2, int W, bool trace);
void sim_destroy(Sim* s);

/* Input */
int  sim_load_jobs(Sim* s, FILE* in);

/* Execution */
void sim_run(Sim* s);

/* Metrics */
void sim_metrics(const Sim* s, double* avg_tat, double* avg_wait, double* avg_resp);

#endif
