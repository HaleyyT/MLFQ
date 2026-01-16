#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/types.h>

#include "proc.h"

int proc_launch(Job *j) {
    pid_t p = fork();
    if (p < 0) {
        perror("fork");
        return -1;
    }
    if (p == 0) {
        // child
        execl(SIGTASK_PATH, SIGTASK_PATH, (char *)NULL);
        // if we get here, exec failed
        perror("execl");
        _exit(1);
    }
    // parent
    j->pid = p;
    return 0;
}

int proc_resume(Job *j) {
    if (j->pid <= 0) return -1;
    if (kill(j->pid, SIGCONT) < 0) {
        perror("kill(SIGCONT)");
        return -1;
    }
    return 0;
}

int proc_stop(Job *j) {
    if (j->pid <= 0) return -1;
    if (kill(j->pid, SIGTSTP) < 0) {
        perror("kill(SIGTSTP)");
        return -1;
    }
    return 0;
}

int proc_terminate(Job *j) {
    if (j->pid <= 0) return -1;
    if (kill(j->pid, SIGINT) < 0) {
        perror("kill(SIGINT)");
        return -1;
    }
    // wait for it to actually die
    waitpid(j->pid, NULL, 0);
    return 0;
}
