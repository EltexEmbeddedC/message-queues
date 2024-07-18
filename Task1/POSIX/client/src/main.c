#include "../include/queue_client.h"

int main() {
    mqd_t mq1, mq2;
    char buffer[MAX_SIZE];

    mq1 = mq_open(SERVER_QUEUE, O_RDONLY);
    if (mq1 == -1) {
        perror("mq_open");
        exit(1);
    }

    mq2 = mq_open(CLIENT_QUEUE, O_WRONLY);
    if (mq2 == -1) {
        perror("mq_open");
        exit(1);
    }

    if (mq_receive(mq1, buffer, MAX_SIZE, NULL) == -1) {
        perror("mq_receive");
        exit(1);
    }
    printf("Received message from server: %s\n", buffer);

    strcpy(buffer, "Hello!");
    if (mq_send(mq2, buffer, strlen(buffer) + 1, 0) == -1) {
        perror("mq_send");
        exit(1);
    }

    if (mq_close(mq1) == -1) {
        perror("mq_close");
        exit(1);
    }

    if (mq_close(mq2) == -1) {
        perror("mq_close");
        exit(1);
    }

    return 0;
}
