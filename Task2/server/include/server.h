#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mqueue.h>
#include <sys/types.h>
#include <sys/msg.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <malloc.h>
#include <sys/ipc.h>
#include <errno.h>
#include <sys/select.h>

#define REG_QUEUE_NAME "reg_queue"
#define MAX_NAME_LEN 32
#define MAX_MSG_LEN 128
#define MAX_DATE_LEN 32

typedef enum { CLIENT = 1, SERVER = 2 } SENDER;
typedef enum {REG = 1, DISCONNECT = 2, MSGINFO = 3, USERINFO = 4 } MSG_TYPE;

typedef struct {
    char username[MAX_NAME_LEN];
    char message[MAX_MSG_LEN];
    char datetime[MAX_DATE_LEN];
} Message;

typedef struct {
    char username[MAX_NAME_LEN];
    mqd_t desc;
    int id;
} User;

typedef struct {
    long mtype;

    int msg_type;
    mqd_t desc;
    int id;
    char username[MAX_NAME_LEN];
    char message[MAX_MSG_LEN];
    char datetime[MAX_DATE_LEN];
} Msgbuf;

void run_server();
void handle_events();
void cleanup_and_exit();
void handle_sigint(int sig);
void create_file(const char * filename);
int register_user(User user);
void add_user(User user);
void send_data_to_new_user(User user);
void add_message(Message msg);
void disconnect_user(User user);
void remove_user(int id);
void send_broadcast(Msgbuf buf, int exeption_id);
void sleep_for_milliseconds(long milliseconds);

#endif // SERVER_H