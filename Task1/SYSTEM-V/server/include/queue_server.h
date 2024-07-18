#ifndef QUEUE_SERVER_H
#define QUEUE_SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>

#define QUEUE_PERMISSIONS 0666
#define N 100

struct msgbuf {
    long mtype;
    char mtext[N];
};

#endif // QUEUE_SERVER_H