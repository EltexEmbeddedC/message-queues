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

User user_info;

User* users;
int users_size;

Message* messages;
int messages_size;

int reg_queue;

void handle_events();
void cleanup_and_exit();
void handle_sigint(int sig);
void handle_events();
void add_user(User user);
void add_message(Message message);

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <client_name>\n", argv[0]);
        exit(1);
    }

    signal(SIGINT, handle_sigint);

    Msgbuf reg_message, resp;
    key_t reg_key = ftok(REG_QUEUE_NAME, 10);

    if (reg_key == -1) {
        perror("ftok");
        exit(1);
    }

    reg_queue = msgget(reg_key, 0666 | IPC_CREAT);
    if (reg_queue == -1) {
        perror("msgget");
        exit(1);
    }

    printf("Client started on desc = %d...\n", reg_queue);

    reg_message.mtype = CLIENT;
    reg_message.msg_type = REG;
    strcpy(reg_message.username, argv[1]);
    reg_message.id = getpid();

    if (msgsnd(reg_queue, &reg_message, sizeof(Msgbuf) - sizeof(long), 0) == -1) {
        perror("msgsnd");
        exit(1);
    }

    if (msgrcv(reg_queue, &resp, sizeof(Msgbuf) - sizeof(long), SERVER, 0) < 0) {
        perror("msgrcv");
        exit(1);
    }

    user_info.desc = resp.desc;
    user_info.id = resp.id;
    strcpy(user_info.username, resp.username);

    printf("Registered with username %s on desc %d\n", user_info.username, user_info.desc);

    handle_events();

    cleanup_and_exit();

    return 0;
}

void cleanup_and_exit() {
    Msgbuf exit_message;

    strcpy(exit_message.username, user_info.username);
    exit_message.mtype = CLIENT;
    exit_message.msg_type = DISCONNECT;
    exit_message.id = user_info.id;
    exit_message.desc = user_info.desc;

    if (msgsnd(reg_queue, &exit_message, sizeof(Msgbuf) - sizeof(long), 0) == -1) {
        perror("msgsnd");
        exit(1);
    }

    printf("Exiting...\n");
    exit(0);
}

void handle_sigint(int sig) {
    cleanup_and_exit();
}

void handle_events(){
    Msgbuf msg;
    User user;
    Message message;

    while (1) {
        if (msgrcv(user_info.desc, &msg, sizeof(Msgbuf) - sizeof(long), SERVER, IPC_NOWAIT) >= 0) {
            switch (msg.msg_type) {
                case MSGINFO:
                    printf("new msg: %s\n", msg.username);

                    strcpy(message.username, msg.username);
                    strcpy(message.message, msg.message);
                    strcpy(message.datetime, msg.datetime);

                    add_message(message);
                    break;
                case USERINFO:
                    printf("new user: %s\n", msg.username);

                    strcpy(user.username, msg.username);
                    user.id = msg.id;
                    user.desc = msg.desc;

                    add_user(user);
                    break;
                case DISCONNECT:
                    printf("user %s disconnected\n", msg.username);
                    remove_user(msg.id);
                    break;
            }
        }
    }
}

void add_user(User user) {
  User* temp = (User*)realloc(users, (++users_size) * sizeof(User));
  if (temp != NULL || (temp == NULL && users_size == 0)) {
    users = temp;
  } else {
    perror("realloc");
    free(users);
    users = NULL;
  }

  strcpy(users[users_size - 1].username, user.username);
  users[users_size - 1].desc = user.desc;
  users[users_size - 1].id = user.id;
}

void add_message(Message message) {
  Message* temp = (Message*)realloc(messages, (++messages_size) * sizeof(Message));
  if (temp != NULL || (temp == NULL && messages_size == 0)) {
    messages = temp;
  } else {
    perror("realloc");
    free(messages);
    users = NULL;
  }

  strcpy(messages[messages_size - 1].username, message.username);
  strcpy(messages[messages_size - 1].message, message.message);
  strcpy(messages[messages_size - 1].datetime, message.datetime);
}

void remove_user(int id) {
    int index = -1;
    for (int i = 0; i < users_size; ++i) {
        if (users[i].id == id) {
            index = i;
            break;
        }
    }

    if (index == -1) {
        perror("User not found");
        return;
    }

    for (int i = index; i < users_size - 1; ++i) {
        users[i] = users[i + 1];
    }

    User* temp = (User*)realloc(users, (--users_size) * sizeof(User));
    if (temp != NULL || users_size == 0) {
        users = temp;
    } else {
        perror("realloc");
        free(users);
        users = NULL;
    }
}
