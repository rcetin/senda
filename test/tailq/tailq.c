#include <sys/queue.h>
#include <stdio.h>

#include "tailq.h"

int main(void)
{
    struct fooq q;

    struct foo f1 = {.tailq = {0, 0}, .datum = 1};
    struct foo f2 = {.tailq = {0, 0}, .datum = 2};
    struct foo f3 = {.tailq = {0, 0}, .datum = 3};

    TAILQ_INIT(&q);
    TAILQ_INSERT_TAIL(&q, &f1, tailq);
    TAILQ_INSERT_TAIL(&q, &f2, tailq);
    TAILQ_INSERT_TAIL(&q, &f3, tailq);

    struct foo *p;
    TAILQ_FOREACH(p, &q, tailq) {
        printf(" %d", p->datum);
    }

    return 0;
}
