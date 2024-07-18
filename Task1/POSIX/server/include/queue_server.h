#ifndef QUEUE_SERVER_H
#define QUEUE_SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <errno.h>

#define QUEUE_PERMISSIONS 0666
#define SERVER_QUEUE "/server"
#define CLIENT_QUEUE "/client"
#define MAX_SIZE 1024

#endif // QUEUE_SERVER_H