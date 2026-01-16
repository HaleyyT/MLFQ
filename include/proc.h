#ifndef PROC_H
#define PROC_H

#include <sys/types.h>
#include "sim.h"   // for Job

#define SIGTASK_PATH "./bin/sigtrap"

int proc_launch(Job *j);          // first time: fork+exec
int proc_resume(Job *j);          // send SIGCONT
int proc_stop(Job *j);            // send SIGTSTP
int proc_terminate(Job *j);       // send SIGINT + wait

#endif
