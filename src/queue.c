#include <stdlib.h>
#include <assert.h>
#include "queue.h"

typedef struct Node {
    struct Node* next;
    struct Job*  val;
} Node;

struct Q {
    Node* head;
    Node* tail;
    size_t sz;
};

Q* q_create(void) {
    Q* q = (Q*)calloc(1, sizeof(Q));
    return q;
}

void q_destroy(Q* q) {
    if (!q) return;
    Node* n = q->head;
    while (n) {
        Node* nx = n->next;
        free(n);
        n = nx;
    }
    free(q);
}

bool q_empty(const Q* q) { return !q || q->sz == 0; }
size_t q_size(const Q* q) { return q ? q->sz : 0; }

void q_push_tail(Q* q, Job* j) {
    Node* n = (Node*)malloc(sizeof(Node));
    n->next = NULL;
    n->val = j;
    if (!q->tail) {
        q->head = q->tail = n;
    } else {
        q->tail->next = n;
        q->tail = n;
    }
    q->sz++;
}

void q_push_head(Q* q, Job* j) {
    Node* n = (Node*)malloc(sizeof(Node));
    n->val = j;
    n->next = q->head;
    q->head = n;
    if (!q->tail) q->tail = n;
    q->sz++;
}

Job* q_pop_head(Q* q) {
    if (!q || !q->head) return NULL;
    Node* n = q->head;
    Job* v = n->val;
    q->head = n->next;
    if (!q->head) q->tail = NULL;
    free(n);
    q->sz--;
    return v;
}

Job* q_peek_head(const Q* q) {
    return (q && q->head) ? q->head->val : NULL;
}
