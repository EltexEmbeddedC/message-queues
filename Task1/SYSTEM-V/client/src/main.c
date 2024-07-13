#include "../include/queue_client.h"

int main() {
    key_t key;
    int msgid;
    struct msgbuf message;

    key = ftok("my_queue", 50);

    msgid = msgget(key, QUEUE_PERMISSIONS);
    if (msgid == -1) {
        perror("msgget");
        exit(1);
    }

    if (msgrcv(msgid, &message, sizeof(message), 1, 0) == -1) {
        perror("msgrcv");
        exit(1);
    }

    printf("Received message from server: %s\n", message.mtext);

    message.mtype = 2;
    strcpy(message.mtext, "Hello!");
    if (msgsnd(msgid, &message, sizeof(message), 0) == -1) {
        perror("msgsnd");
        exit(1);
    }

    return 0;
}
