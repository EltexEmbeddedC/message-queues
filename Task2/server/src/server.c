#include "../include/server.h"

User* users;
int users_size;

Message* messages;
int messages_size;

int reg_queue;
int id;

void run_server() {
    signal(SIGINT, handle_sigint);

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
    Message message;

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
                    msg.mtype = SERVER;
                    send_broadcast(msg, users[i].id);

                    strcpy(message.username, msg.username);
                    strcpy(message.message, msg.message);
                    strcpy(message.datetime, msg.datetime);
                    add_message(message);
                    break;
                }
            }
        }
        sleep_for_milliseconds(10);
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

void sleep_for_milliseconds(long milliseconds) {
    struct timeval tv;
    tv.tv_sec = milliseconds / 1000;
    tv.tv_usec = (milliseconds % 1000) * 1000;
    select(0, NULL, NULL, NULL, &tv);
}
