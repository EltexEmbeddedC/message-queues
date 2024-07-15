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

User* users;
int users_size;

Message* messages;
int messages_size;

int reg_queue;
int id;

void handle_events();
void cleanup_and_exit();
void handle_sigint(int sig);
void create_file(const char * filename);
int register_user(User user);
void add_user(User user);
void demo_data();
void send_data_to_new_user(User user);
void add_message(Message msg);
void disconnect_user(User user);
void remove_user(int id);

int main() {
    signal(SIGINT, handle_sigint);
    demo_data();

    create_file(REG_QUEUE_NAME);
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

    printf("Server started and waiting for client registrations on desc = %d...\n", reg_queue);

    handle_events();

    cleanup_and_exit();

    return 0;
}

void demo_data(){
    Message m1, m2, m3;
    
    strcpy(m1.username, "user1");
    strcpy(m1.message, "msg1");
    strcpy(m1.datetime, "15.07.2024");
    
    strcpy(m2.username, "user2");
    strcpy(m2.message, "msg2");
    strcpy(m2.datetime, "16.07.2024");

    strcpy(m3.username, "user3");
    strcpy(m3.message, "msg3");
    strcpy(m3.datetime, "17.07.2024");

    add_message(m1);
    add_message(m2);
    add_message(m3);
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

void handle_events() {
    Msgbuf msg;
    User user;

    while (1) {
        if (msgrcv(reg_queue, &msg, sizeof(Msgbuf) - sizeof(long), CLIENT, IPC_NOWAIT) >= 0) {
            switch (msg.msg_type) {
                case REG:
                    printf("User %s asks for registration.\n", msg.username);
                    strcpy(user.username, msg.username);
                    register_user(user);
                    break;
                case DISCONNECT:
                    printf("User %s has disconnected.\n", msg.username);
                    strcpy(user.username, msg.username);
                    user.id = msg.id;
                    user.desc = msg.desc;
                    disconnect_user(user);
                    break;
            }
        }
        for (int i = 0; i < users_size; i++){
            if (msgrcv(users[i].desc, &msg, sizeof(Msgbuf) - sizeof(long), CLIENT, IPC_NOWAIT) >= 0) {
            switch (msg.msg_type) {
                case MSGINFO:
                    printf("Received message from %s: %s\n", msg.username, msg.message);
                    break;
                case USERINFO:
                    printf("User %s has joined the chat.\n", msg.username);
                    break;
                }
            }
        }
    }
}

void send_broadcast(Msgbuf buf, int exeption_id){
    for (int i = 0; i < users_size; i++){
        if (users[i].id == exeption_id){
            continue;
        }

        if (msgsnd(users[i].desc, &buf, sizeof(Msgbuf) - sizeof(long), 0) == -1) {
            perror("msgsnd");
            exit(1);
        }
    }
}

void disconnect_user(User user){
    Msgbuf buf;

    buf.mtype = SERVER;
    buf.msg_type = DISCONNECT;
    buf.id = user.id;
    buf.desc = user.desc;
    strcpy(buf.username, user.username);

    remove_user(user.id);
    send_broadcast(buf, -1);
}

int register_user(User user){
    Msgbuf resp;

    user.id = id++;
    char filename[20] = "";
    sprintf(filename, "user_%d", user.id);
    create_file(filename);

    key_t key = ftok(filename, 10);
    if (key == -1) {
        perror("ftok");
        exit(1);
    }

    user.desc = msgget(key, 0666 | IPC_CREAT);
    if (user.desc == -1) {
        perror("msgget");
        exit(1);
    }

    add_user(user);

    resp.desc = user.desc;
    resp.mtype = SERVER;
    resp.msg_type = USERINFO;
    resp.id = user.id;
    strcpy(resp.username, user.username);


    if (msgsnd(reg_queue, &resp, sizeof(Msgbuf) - sizeof(long), 0) == -1) {
        perror("msgsnd");
        exit(1);
    }

    send_data_to_new_user(users[users_size - 1]);
    send_broadcast(resp, user.id);

    return user.desc;
}

void send_data_to_new_user(User user){
    Msgbuf resp;

    for (int i = 0; i < messages_size; i++) {
        strcpy(resp.message, messages[i].message);
        strcpy(resp.username, messages[i].username);
        strcpy(resp.datetime, messages[i].datetime);
        resp.mtype = SERVER;
        resp.msg_type = MSGINFO;

        if (msgsnd(user.desc, &resp, sizeof(Msgbuf) - sizeof(long), 0) == -1) {
            perror("msgsnd");
            exit(1);
        }
    }

    for (int i = 0; i < users_size; i++) {
        strcpy(resp.username, users[i].username);
        resp.id = users[i].id;
        resp.desc = users[i].desc;
        resp.mtype = SERVER;
        resp.msg_type = USERINFO;

        if (msgsnd(user.desc, &resp, sizeof(Msgbuf) - sizeof(long), 0) == -1) {
            perror("msgsnd");
            exit(1);
        }
    }
}

void cleanup_and_exit() {
    if (msgctl(reg_queue, IPC_RMID, NULL) == -1) {
        perror("msgctl");
        exit(1);
    }

    printf("Server stopped. Cleaning up and exiting...\n");
    exit(0);
}

void create_file(const char * filename){
    if (access(filename, F_OK) == -1) {
        int fd = open(filename, O_CREAT | O_EXCL, 0666);
        if (fd == -1) {
            perror("open");
            exit(1);
        }
        close(fd);
    }
}

void handle_sigint(int sig) {
    cleanup_and_exit();
}
