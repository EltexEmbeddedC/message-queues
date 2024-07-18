#include "../include/queue_server.h"

int main() {
    mqd_t mq1, mq2;
    struct mq_attr attr;
    char buffer[MAX_SIZE];

    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = MAX_SIZE;
    attr.mq_curmsgs = 0;

    mq1 = mq_open(SERVER_QUEUE, O_CREAT | O_WRONLY, QUEUE_PERMISSIONS, &attr);
    if (mq1 == -1) {
        perror("mq_open");
        exit(1);
    }

    mq2 = mq_open(CLIENT_QUEUE, O_CREAT | O_RDONLY, QUEUE_PERMISSIONS, &attr);
    if (mq2 == -1) {
        perror("mq_open");
        exit(1);
    }

    strcpy(buffer, "Hi!");
    if (mq_send(mq1, buffer, strlen(buffer) + 1, 0) == -1) {
        perror("mq_send");
        exit(1);
    }

    if (mq_receive(mq2, buffer, MAX_SIZE, NULL) == -1) {
        perror("mq_receive");
        exit(1);
    }
    printf("Received response from client: %s\n", buffer);

    if (mq_close(mq1) == -1) {
        perror("mq_close");
        exit(1);
    }
    if (mq_unlink(SERVER_QUEUE) == -1) {
        perror("mq_unlink");
        exit(1);
    }

    if (mq_close(mq2) == -1) {
        perror("mq_close");
        exit(1);
    }
    if (mq_unlink(CLIENT_QUEUE) == -1) {
        perror("mq_unlink");
        exit(1);
    }

    return 0;
}
