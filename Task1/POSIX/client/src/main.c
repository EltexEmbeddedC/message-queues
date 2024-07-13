#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <errno.h>

#define QUEUE_NAME "/my_queue"
#define MAX_SIZE 1024
#define SERVER_PRIORITY 1
#define CLIENT_PRIORITY 2

int main() {
    mqd_t mq;
    char buffer[MAX_SIZE];

    // Открытие очереди сообщений
    mq = mq_open(QUEUE_NAME, O_RDWR);
    if (mq == -1) {
        perror("mq_open");
        exit(1);
    }

    // Получение сообщения от сервера с приоритетом сервера
    if (mq_receive(mq, buffer, MAX_SIZE, NULL) == -1) {
        perror("mq_receive");
        exit(1);
    }
    printf("Клиент: Получено сообщение '%s'\n", buffer);

    // Отправка ответа серверу с более высоким приоритетом
    strcpy(buffer, "Hello!");
    if (mq_send(mq, buffer, strlen(buffer) + 1, CLIENT_PRIORITY) == -1) {
        perror("mq_send");
        exit(1);
    }
    printf("Клиент: Отправлено сообщение '%s'\n", buffer);

    // Закрытие очереди сообщений
    if (mq_close(mq) == -1) {
        perror("mq_close");
        exit(1);
    }

    return 0;
}
