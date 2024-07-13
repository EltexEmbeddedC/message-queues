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
    struct mq_attr attr;
    char buffer[MAX_SIZE];

    // Настройки очереди сообщений
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = MAX_SIZE;
    attr.mq_curmsgs = 0;

    // Создание очереди сообщений
    mq = mq_open(QUEUE_NAME, O_CREAT | O_RDWR, 0644, &attr);
    if (mq == -1) {
        perror("mq_open");
        exit(1);
    }

    // Отправка сообщения клиенту
    strcpy(buffer, "Hi!");
    if (mq_send(mq, buffer, strlen(buffer) + 1, SERVER_PRIORITY) == -1) {
        perror("mq_send");
        exit(1);
    }
    printf("Сервер: Отправлено сообщение '%s'\n", buffer);

    // Ожидание ответа от клиента с более высоким приоритетом
    if (mq_receive(mq, buffer, MAX_SIZE, NULL) == -1) {
        perror("mq_receive");
        exit(1);
    }
    printf("Сервер: Получено сообщение '%s'\n", buffer);

    // Закрытие и удаление очереди сообщений
    if (mq_close(mq) == -1) {
        perror("mq_close");
        exit(1);
    }
    if (mq_unlink(QUEUE_NAME) == -1) {
        perror("mq_unlink");
        exit(1);
    }

    return 0;
}
