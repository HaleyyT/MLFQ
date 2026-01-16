#ifndef QUEUE_H
#define QUEUE_H

#include <stdbool.h>
#include <stddef.h>

typedef struct Job Job;
typedef struct Q Q;

Q*  q_create(void);
void q_destroy(Q* q);

bool   q_empty(const Q* q);
size_t q_size(const Q* q);

void   q_push_tail(Q* q, Job* j);
void   q_push_head(Q* q, Job* j);
Job*   q_pop_head(Q* q);
Job*   q_peek_head(const Q* q);

#endif
