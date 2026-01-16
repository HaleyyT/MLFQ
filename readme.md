# Multilevel Feedback Queue (MLFQ) CPU Scheduler — Systems Project

Design and implementation of a spec-accurate CPU scheduler that simulates a production-style, preemptive Multilevel Feedback Queue with starvation-free aging and precise per-tick semantics.

- TL;DR: I built a small “OS scheduler” that enforces priorities, preemption, round-robin (RR), and anti-starvation aging—then proved correctness with traceable, deterministic runs and average latency metrics.

## Why this project matters
Modern kernels use multi-level priority queues and time quanta to balance latency for interactive tasks with throughput for batch work. Getting this right requires careful handling of:

- Preemption at the exact tick a higher-priority task appears
- Quantum expiry on tick boundaries (no “time travel”)
- Aging that actually prevents starvation (measured from last tail enqueue)
- Deterministic traces to verify behavior and debug timing bugs
- This project demonstrates my ability to translate a written spec into a correct, testable, and maintainable systems implementation.

## Highlights
**Three levels:**
- **L0** (highest): fixed quantum, demotes to L1
- **L1:** fixed quantum, preemptible by L0, demotes to L2
- **L2:** round-robin with quantum, preemptible by L0/L1

**Starvation-free aging (W):** boosts queues when the front job’s wait since last tail insert ≥ W
**If L1 front ages:** promote all L1, then all L2 → L0. **Else if L2 front ages:** promote all L2 → L0
**Correct per-tick order:** arrivals → aging → preemption → dispatch → run → finish/quantum → t++
**Metrics:** average Turnaround, Waiting, Response (two-decimal output)
Clean architecture: small queue ADT, clear helpers (demote/rotate/preempt), compact Sim state
**Traceable:** every decision is logged (ARRIVE, DISPATCH, RUN, QEXP, PREEMPT, FINISH) for debugging and assessment

## What I solved
**Fixed a subtle timing bug:** preemption was originally checked after running the tick, causing logs at t+1. Reordered the loop so preemption happens before CPU time is consumed.

**Aging that really works:** the trigger compares t - head->last_enqueue_time (time since last tail enqueue), and I only update last_enqueue_time on tail inserts/promotions—never on head requeues—so waiting time grows predictably.

**Consistent timestamps:**
- Arrival enqueues at t
- Demote/rotate enqueues at t+1 (end of tick)
- Aging promotions at t (start of tick)
- Deterministic RR: L2 quantum expiry requeues to tail with a fresh quantum.

## Quick start
bash 
```
# Build
make         # builds ./bin/sim and ./sigtrap (child demo program)

# Run (interactive prompts)
./bin/sim

# Or CLI args if you prefer
./bin/sim 4 3 2 2 tests/f_aging_L1_trigger.txt --trace
```


**Sample trace excerpt** (L1 preempted by L0; L2 round-robins):
t=0 ARRIVE job=J0 -> L2
t=0 DISPATCH job=J0 from=L2
t=2 QEXP job=J0 L2 used=2 -> L2(tail)
t=3 ARRIVE job=J1 -> L0
t=3 PREEMPT job=J0 by L0/L1 arrival -> L2(head)
t=3 DISPATCH job=J1 from=L0
...

## Design at a glance
**Language:** C (C11), single-threaded simulator
**Data structures:** O(1) FIFO queues with head/tail

**State:**
dispatch, l0, l1, l2 queues
t0/t1/t2 (quanta), W (aging threshold)
global t (ticks), running job pointer

**Key helpers:**
demote_after_quantum_L0/L1, rr_rotate_L2_tail
preempt_requeue_head (no credit lost, keeps partial quantum)
apply_aging (front-based wait, strong L1-then-L2 rule)

**Metrics:**
TAT = finish - arrival
RESP = first_start - arrival
WAIT = TAT - cpu_total

## Testing & Verification
Trace-driven tests that match expected behavior:
- b_L1_preempt_by_L0.txt – L1 is preempted the instant an L0 job arrives
- e_L2_rr_preempt.txt – L2 round-robins and is preempted by L0/L1
- f_aging_L1_trigger.txt / g_aging_L2_trigger.txt – W aging promotions fire exactly when they should

**Metric checks:** printed averages align with hand-computed values
**Sanitizers:** make SAN=1 for AddressSanitizer builds

## What I learned
- How small ordering mistakes (preemption after run) create off-by-one timing errors that cascade through a scheduler.
- Implementing aging correctly requires discipline about when you mutate “wait since tail” state.
- Writing deterministic traces is invaluable for debugging specs that depend on exact tick boundaries.
- Possible extensions
- Add pid_t to jobs and integrate a tiny child process (sigtrap) to demonstrate SIGTSTP/SIGCONT handling.
- Export traces as JSON/CSV and write a diff tool for grading/regression tests.
- Add I/O blocking and wake-ups to simulate mixed CPU/I/O workloads.

## Repo structure
bash 
```
    include/   # sim.h, queue.h, util.h, proc.h
    src/       # sim_core.c, queue.c, util.c, main.c, sigtrap.c, proc.c, 
    tests/     # input cases used in verification
    Makefile
    bin/obj/   # build output

```