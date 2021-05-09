#ifndef _TAILQ_H
#define _TAILQ_H

#include <sys/queue.h>

struct foo {
    TAILQ_ENTRY(foo) tailq;
    int datum;
};

TAILQ_HEAD(fooq, foo);

#endif