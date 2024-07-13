#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <time.h>
#include <errno.h>

#define MAX_NAME_LEN 32
#define MAX_MSG_LEN 512
#define MAX_DATE_LEN 64
#define SERVER_QUEUE_KEY 1234

typedef struct {
    long msg_type;
    char username[MAX_NAME_LEN];
    char message[MAX_MSG_LEN];
    char datetime[MAX_DATE_LEN];
} UserMessage;

typedef struct {
    char username[MAX_NAME_LEN];
    int queue_id;
} User;

typedef struct {
    long msg_type;
    char username[MAX_NAME_LEN];
} UserConnectMessage;

User *users = NULL;
int user_count = 0;
int server_queue_id;

pthread_mutex_t user_mutex = PTHREAD_MUTEX_INITIALIZER;

void add_user(const char *username, int queue_id) {
    pthread_mutex_lock(&user_mutex);
    users = realloc(users, sizeof(User) * (user_count + 1));
    strncpy(users[user_count].username, username, MAX_NAME_LEN);
    users[user_count].queue_id = queue_id;
    user_count++;
    pthread_mutex_unlock(&user_mutex);
}

void send_message_to_all(UserMessage *msg) {
    pthread_mutex_lock(&user_mutex);
    for (int i = 0; i < user_count; i++) {
        msgsnd(users[i].queue_id, msg, sizeof(UserMessage) - sizeof(long), 0);
    }
    pthread_mutex_unlock(&user_mutex);
}

void *client_handler(void *arg) {
    int client_queue_id = *((int *)arg);
    free(arg);

    UserMessage msg;
    while (1) {
        if (msgrcv(client_queue_id, &msg, sizeof(UserMessage) - sizeof(long), 0, 0) >= 0) {
            send_message_to_all(&msg);
        }
    }
}

int main() {
    server_queue_id = msgget(SERVER_QUEUE_KEY, IPC_CREAT | 0666);
    if (server_queue_id == -1) {
        perror("msgget");
        exit(EXIT_FAILURE);
    }

    while (1) {
        UserConnectMessage connect_msg;
        if (msgrcv(server_queue_id, &connect_msg, sizeof(UserConnectMessage) - sizeof(long), 1, 0) >= 0) {
            int client_queue_id = msgget(connect_msg.username[0], IPC_CREAT | 0666);
            if (client_queue_id == -1) {
                perror("msgget");
                continue;
            }

            add_user(connect_msg.username, client_queue_id);

            UserMessage welcome_msg;
            welcome_msg.msg_type = 1;
            strncpy(welcome_msg.username, "Server", MAX_NAME_LEN);
            snprintf(welcome_msg.message, MAX_MSG_LEN, "%s has joined the chat.", connect_msg.username);
            time_t now = time(NULL);
            strftime(welcome_msg.datetime, MAX_DATE_LEN, "%Y-%m-%d %H:%M:%S", localtime(&now));

            send_message_to_all(&welcome_msg);

            int *arg = malloc(sizeof(int));
            *arg = client_queue_id;
            pthread_t thread;
            pthread_create(&thread, NULL, client_handler, arg);
            pthread_detach(thread);
        }
    }

    return 0;
}
