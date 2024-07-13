#include "../include/queue_server.h"

int main() {
    key_t key;
    int msgid;
    struct msgbuf message;

    key = ftok("my_queue", 50);

    msgid = msgget(key, IPC_CREAT | QUEUE_PERMISSIONS);
    if (msgid == -1) {
        perror("msgget");
        exit(1);
    }

    message.mtype = 1;
    strcpy(message.mtext, "Hi!");
    if (msgsnd(msgid, &message, sizeof(message), 0) == -1) {
        perror("msgsnd");
        exit(1);
    }

    if (msgrcv(msgid, &message, sizeof(message), 2, 0) == -1) {
        perror("msgrcv");
        exit(1);
    }

    printf("Received response from client: %s\n", message.mtext);

    if (msgctl(msgid, IPC_RMID, NULL) == -1) {
        perror("msgctl");
        exit(1);
    }

    return 0;
}
