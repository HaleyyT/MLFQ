#include <stdlib.h>
#include <unistd.h> 
#include <string.h>
#include <limits.h>
#include "sim.h"
#include "queue.h"
#include "util.h"
#include "proc.h"


//     ---- helpers ---

static int quantum_for_level(const Sim* s, int level) {
    return (level == 0) ? s->t0
         : (level == 1) ? s->t1
         : s->t2;
}

// L0 quantum expiry -> demote to L1 tail (end of tick => t+1)
static void demote_after_quantum_L0(Sim* s, Job* j) {
    j->curr_level = 1;
    j->time_used_in_quantum = 0;
    j->last_enqueue_time = s->t + 1;
    q_push_tail(s->l1, j);
    tracef(s->trace, s->t + 1,
           "QEXP job=J%d L0 used=%d -> L1(tail)\n",
           j->id, s->t0);
}

// L1 quantum expiry -> demote to L2 tail 
static void demote_after_quantum_L1(Sim* s, Job* j) {
    j->curr_level = 2;
    j->time_used_in_quantum = 0;
    j->last_enqueue_time = s->t + 1;
    q_push_tail(s->l2, j);
    tracef(s->trace, s->t + 1,
           "QEXP job=J%d L1 used=%d -> L2(tail)\n",
           j->id, s->t1);
}

// L2 RR quantum expiry -> stay L2 tail
static void rr_rotate_L2_tail(Sim* s, Job* j) {
    j->time_used_in_quantum = 0;
    j->last_enqueue_time = s->t + 1;
    q_push_tail(s->l2, j);
    tracef(s->trace, s->t + 1,
           "QEXP job=J%d L2 used=%d -> L2(tail)\n",
           j->id, s->t2);
}

// Preempt running job to HEAD of its current queue
static void preempt_requeue_head(Sim* s, Job* j) {
    if (j->curr_level == 0) {
        q_push_head(s->l0, j);
    } else if (j->curr_level == 1) {
        q_push_head(s->l1, j);
    } else {
        q_push_head(s->l2, j);
    }
}

//    --- job / sim setup ---  

static Job* job_new(int id, int arrival, int cpu_total, int prio) {
    Job* j = calloc(1, sizeof(Job));
    j->id             = id;
    j->arrival        = arrival;
    j->cpu_total      = cpu_total;
    j->cpu_remaining  = cpu_total;
    j->init_prio      = prio;
    j->curr_level     = prio;
    j->time_used_in_quantum = 0;
    j->first_start_time     = -1;
    j->finish_time          = -1;
    j->last_enqueue_time    = arrival;  
    j->has_started          = false;
    return j;
}

void sim_init(Sim* s, int t0, int t1, int t2, int W, bool trace) {
    s->dispatch = q_create();
    s->l0 = q_create();
    s->l1 = q_create();
    s->l2 = q_create();
    s->running = NULL;

    s->t    = 0;
    s->t0   = t0;
    s->t1   = t1;
    s->t2   = t2;
    s->W    = W;
    s->trace = trace;

    s->job_count  = 0;
    s->total_tat  = 0.0;
    s->total_wait = 0.0;
    s->total_resp = 0.0;
}

void sim_destroy(Sim* s) {
    q_destroy(s->dispatch);
    q_destroy(s->l0);
    q_destroy(s->l1);
    q_destroy(s->l2);
}

int sim_load_jobs(Sim* s, FILE* in) {
    int a, cpu, p;
    int id = 0;
    int count = 0;
    while (fscanf(in, " %d , %d , %d", &a, &cpu, &p) == 3) {
        Job* j = job_new(id++, a, cpu, p);
        q_push_tail(s->dispatch, j);
        count++;
    }
    s->job_count = (size_t)count;
    return count;
}

//      --- arrivals ---

static void enqueue_arrivals(Sim* s) {
    while (!q_empty(s->dispatch)) {
        Job* j = q_peek_head(s->dispatch);
        if (j->arrival > s->t) break;

        (void)q_pop_head(s->dispatch);

        j->curr_level = j->init_prio;
        j->last_enqueue_time = s->t;      /* arrived at start of tick t */
        j->time_used_in_quantum = 0;

        if (j->curr_level == 0)      q_push_tail(s->l0, j);
        else if (j->curr_level == 1) q_push_tail(s->l1, j);
        else                         q_push_tail(s->l2, j);

        tracef(s->trace, s->t,
               "ARRIVE job=J%d -> L%d\n",
               j->id, j->curr_level);
    }
}


//      --- Aging ---
//move everything from q -> L0(tail)
static void move_all_to_L0_tail(Sim* s, Q* q, int now) {
    while (!q_empty(q)) {
        Job* j = q_pop_head(q);
        j->curr_level = 0;
        j->time_used_in_quantum = 0;
        j->last_enqueue_time = now;   /* promotion happens now */
        q_push_tail(s->l0, j);
    }
}

static void apply_aging(Sim* s) {
    if (s->W <= 0) return;

    /* check L1 head first */
    Job* h1 = q_peek_head(s->l1);
    if (h1 && (s->t - h1->last_enqueue_time) >= s->W) {
        move_all_to_L0_tail(s, s->l1, s->t);
        move_all_to_L0_tail(s, s->l2, s->t);
        tracef(s->trace, s->t,
               "AGING L1front>=W: move L1,L2 -> L0(tail)\n");
        return;
    }

    /* else check L2 head */
    Job* h2 = q_peek_head(s->l2);
    if (h2 && (s->t - h2->last_enqueue_time) >= s->W) {
        move_all_to_L0_tail(s, s->l2, s->t);
        tracef(s->trace, s->t,
               "AGING L2front>=W: move L2 -> L0(tail)\n");
    }
}

//pick next
static Job* pick_next(Sim* s) {
    if (!q_empty(s->l0)) return q_pop_head(s->l0);
    if (!q_empty(s->l1)) return q_pop_head(s->l1);
    if (!q_empty(s->l2)) return q_pop_head(s->l2);
    return NULL;
}

//metrics 
void sim_metrics(const Sim* s,
                 double* avg_tat,
                 double* avg_wait,
                 double* avg_resp) {
    if (s->job_count == 0) {
        *avg_tat = *avg_wait = *avg_resp = 0.0;
        return;
    }
    *avg_tat  = s->total_tat  / (double)s->job_count;
    *avg_wait = s->total_wait / (double)s->job_count;
    *avg_resp = s->total_resp / (double)s->job_count;
}


//     ---  main loop --- 

void sim_run(Sim* s) {
    while (s->running ||
           !q_empty(s->dispatch) ||
           !q_empty(s->l0) ||
           !q_empty(s->l1) ||
           !q_empty(s->l2)) {

        // 1. arrivals + aging 
        enqueue_arrivals(s);
        apply_aging(s);

        // 2. preempt running job if higher level ready 
        if (s->running) {
            int lvl = s->running->curr_level;
            bool l0_has = !q_empty(s->l0);
            bool l1_has = !q_empty(s->l1);

            if (lvl == 1 && l0_has) {
                // dispatcher tells running process to pause
                proc_stop(s->running);
                tracef(s->trace, s->t,
                       "PREEMPT job=J%d by L0 arrival -> L1(head)\n",
                       s->running->id);
                preempt_requeue_head(s, s->running);
                s->running = NULL;
            } else if (lvl == 2 && (l0_has || l1_has)) {
                // pause L2 child when higher level appears
                proc_stop(s->running);
                tracef(s->trace, s->t,
                       "PREEMPT job=J%d by L0/L1 arrival -> L2(head)\n",
                       s->running->id);
                preempt_requeue_head(s, s->running);
                s->running = NULL;
            }
        }

        // 3. dispatch if idle
        if (!s->running) {
            s->running = pick_next(s);
            if (s->running) {
                if (!s->running->has_started) {
                    s->running->has_started = true;
                    s->running->first_start_time = s->t;
                    // START child
                    proc_launch(s->running); 
                } else {
                    proc_resume(s->running);
                }
                tracef(s->trace, s->t,
                       "DISPATCH job=J%d from=L%d\n",
                       s->running->id,
                       s->running->curr_level);
            }
        }

        // 4. idle tick
        if (!s->running) {
            s->t++;
            continue;
        }

        // 5. run exactly one tick
        s->running->cpu_remaining--;
        s->running->time_used_in_quantum++;
        tracef(s->trace, s->t,
               "RUN job=J%d rem=%d used=%d\n",
               s->running->id,
               s->running->cpu_remaining,
               s->running->time_used_in_quantum);

        // 6. end-of-tick: finish or quantum 
        if (s->running->cpu_remaining == 0) {
            s->running->finish_time = s->t + 1;
            int tat  = s->running->finish_time - s->running->arrival;
            int resp = s->running->first_start_time - s->running->arrival;
            int wait = tat - s->running->cpu_total;

            s->total_tat  += tat;
            s->total_wait += wait;
            s->total_resp += resp;

            tracef(s->trace, s->t + 1,
                   "FINISH job=J%d TAT=%d RESP=%d WAIT=%d\n",
                   s->running->id, tat, resp, wait);
            //        
            proc_terminate(s->running);
            free(s->running);
            s->running = NULL;
        } else {
            int lvl = s->running->curr_level;
            int q   = quantum_for_level(s, lvl);
            if (s->running->time_used_in_quantum >= q) {
                proc_stop(s->running);
                if (lvl == 0) {
                    demote_after_quantum_L0(s, s->running);
                    s->running = NULL;
                } else if (lvl == 1) {
                    demote_after_quantum_L1(s, s->running);
                    s->running = NULL;
                } else {
                    rr_rotate_L2_tail(s, s->running);
                    s->running = NULL;
                }
            }
        }
        // advance time 
        s->t++;
        sleep(1);
    }
}
