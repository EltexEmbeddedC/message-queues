#ifndef QUEUE_CLIENT_H
#define QUEUE_CLIENT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <errno.h>

#define SERVER_QUEUE "/server"
#define CLIENT_QUEUE "/client"
#define MAX_SIZE 1024

#endif // QUEUE_CLIENT_H