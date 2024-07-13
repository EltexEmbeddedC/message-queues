#ifndef QUEUE_CLIENT_H
#define QUEUE_CLIENT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#define QUEUE_PERMISSIONS 0660
#define N 100

struct msgbuf {
    long mtype;
    char mtext[N];
};

#endif // QUEUE_CLIENT_H