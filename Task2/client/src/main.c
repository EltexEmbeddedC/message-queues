#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <mqueue.h>
#include <sys/types.h>
#include <sys/msg.h>
#include <unistd.h>
#include <signal.h>
#include <malloc.h>
#include <sys/ipc.h>
#include <errno.h>
#include <pthread.h>
#include <sys/select.h>

#define REG_QUEUE_NAME "reg_queue"
#define MAX_NAME_LEN 32
#define MAX_MSG_LEN 128
#define MAX_DATE_LEN 32

typedef enum { CLIENT = 1, SERVER = 2 } SENDER;
typedef enum {REG = 1, DISCONNECT = 2, MSGINFO = 3, USERINFO = 4 } MSG_TYPE;

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

int reg_queue;

typedef struct {
    char username[MAX_NAME_LEN];
    char message[MAX_MSG_LEN];
    char datetime[MAX_DATE_LEN];
} Message;

Message *messages = NULL;
int messages_size;

User *users = NULL;
int users_size = 0;

char input_message[MAX_MSG_LEN] = "";

WINDOW *msg_win, *user_win, *input_win;
WINDOW *msg_win_border, *user_win_border, *input_win_border;
int msg_win_height, msg_win_width;
int user_win_height, user_win_width;
int input_win_height, input_win_width;

int msg_scroll_pos = 0;
int user_scroll_pos = 0;
int active_window = 0; // 0 - msg_win, 1 - user_win

void init_ncurses();
void create_windows();
void add_message(Message message, bool is_new);
void add_user(User user);
void display_messages();
void display_users();
void display_input();
void* handle_input(void*);
void read_message();
void clean_up();
void switch_window();
void handle_sigint(int sig);
void* handle_events(void*);
void remove_user(int id);
void sleep_for_milliseconds(long milliseconds);

void init_ncurses() {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    start_color();
    init_pair(1, COLOR_CYAN, COLOR_BLACK);  // Username
    init_pair(2, COLOR_GREEN, COLOR_BLACK); // Message
    init_pair(3, COLOR_YELLOW, COLOR_BLACK); // Datetime
    init_pair(4, COLOR_WHITE, COLOR_BLACK); // Input
    init_pair(5, COLOR_WHITE, COLOR_BLACK); // Borders
}

void create_windows() {
    msg_win_height = LINES - 5;
    msg_win_width = COLS - 24;
    user_win_height = LINES - 5;
    user_win_width = 20;
    input_win_height = 4;
    input_win_width = COLS;

    msg_win_border = newwin(msg_win_height + 1, msg_win_width + 2, 0, 0);
    user_win_border = newwin(user_win_height + 1, user_win_width + 2, 0, COLS - user_win_width - 2);
    input_win_border = newwin(input_win_height, input_win_width, LINES - input_win_height, 0);

    msg_win = newwin(msg_win_height - 1, msg_win_width, 1, 1);
    user_win = newwin(user_win_height - 1, user_win_width, 1, COLS - user_win_width - 1);
    input_win = newwin(input_win_height - 2, input_win_width - 2, LINES - input_win_height + 1, 1);

    scrollok(msg_win, TRUE);
    scrollok(user_win, TRUE);
    scrollok(input_win, TRUE);

    box(msg_win_border, 0, 0);
    box(user_win_border, 0, 0);
    box(input_win_border, 0, 0);

    wrefresh(msg_win_border);
    wrefresh(user_win_border);
    wrefresh(input_win_border);
}

void add_message(Message message, bool is_new) {
    Message* temp = (Message*)realloc(messages, (++messages_size) * sizeof(Message));
    if (temp != NULL || (temp == NULL && messages_size == 0)) {
        messages = temp;
    } else {
        free(messages);
        users = NULL;
    }

    strcpy(messages[messages_size - 1].username, message.username);
    strcpy(messages[messages_size - 1].message, message.message);

    if (is_new){
        time_t now = time(NULL);
        strftime(messages[messages_size - 1].datetime, MAX_DATE_LEN, "%Y-%m-%d %H:%M:%S", localtime(&now));

        Msgbuf buf;
        buf.mtype = CLIENT;
        buf.msg_type = MSGINFO;
        strcpy(buf.username, message.username);
        strcpy(buf.datetime, messages[messages_size - 1].datetime);
        strcpy(buf.message, message.message);
        if (msgsnd(user_info.desc, &buf, sizeof(Msgbuf) - sizeof(long), 0) == -1) {
            perror("msgsnd");
            exit(1);
        }
    } else {
        strcpy(messages[messages_size - 1].datetime, message.datetime);
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

    display_users();
}

void display_messages() {
    werase(msg_win);
    int start = messages_size - msg_scroll_pos - msg_win_height / 2;
    if (start < 0) start = 0;
    for (int i = start; i < messages_size; i++) {
        wattron(msg_win, COLOR_PAIR(1));
        wprintw(msg_win, "%s: ", messages[i].username);
        wattroff(msg_win, COLOR_PAIR(1));
        wattron(msg_win, COLOR_PAIR(2));
        wprintw(msg_win, "%s\n", messages[i].message);
        wattroff(msg_win, COLOR_PAIR(2));
        wattron(msg_win, COLOR_PAIR(3));
        wprintw(msg_win, "%s\n", messages[i].datetime);
        wattroff(msg_win, COLOR_PAIR(3));
    }
    wrefresh(msg_win);
}

void display_users() {
    werase(user_win);
    int start = user_scroll_pos;
    for (int i = start; i < users_size; i++) {
        wprintw(user_win, "%s\n", users[i].username);
    }
    wrefresh(user_win);
}

void display_input() {
    werase(input_win);
    wattron(input_win, COLOR_PAIR(4));
    mvwprintw(input_win, 1, 1, "%s", input_message);
    wattroff(input_win, COLOR_PAIR(4));
    wrefresh(input_win);
}

void* handle_input(void*){
    while (1){
        read_message();
    }
}

void read_message() {
    int ch;
    int input_index = strlen(input_message);

    while ((ch = wgetch(input_win)) != '\n') {
        if (ch == 27) { // Escape key
            clean_up();
            exit(0);
        } else if (ch == KEY_BACKSPACE || ch == 127 || ch == '\b') {
            if (input_index > 0) {
                input_message[--input_index] = '\0';
            }
        } else if (ch == KEY_RESIZE) {
            create_windows();
            display_messages();
            display_users();
            display_input();
        } else if (ch == '\t') { // Tab key
            switch_window();
        } else if (ch == KEY_UP) {
            if (active_window == 0 && msg_scroll_pos < messages_size - msg_win_height / 2) {
                msg_scroll_pos++;
                display_messages();
            } else if (active_window == 1 && user_scroll_pos < users_size - user_win_height) {
                user_scroll_pos++;
                display_users();
            }
        } else if (ch == KEY_DOWN) {
            if (active_window == 0 && msg_scroll_pos > 0) {
                msg_scroll_pos--;
                display_messages();
            } else if (active_window == 1 && user_scroll_pos > 0) {
                user_scroll_pos--;
                display_users();
            }
        } else if (input_index < MAX_MSG_LEN - 1) {
            input_message[input_index++] = ch;
            input_message[input_index] = '\0';
        }
        display_input();
    }

    if (input_index > 0) {
        Message msg;
        strcpy(msg.username, user_info. username);
        strcpy(msg.message, input_message);
        add_message(msg, 1);
        input_message[0] = '\0';
        display_messages();
        display_input();
    }
}

void clean_up() {
    free(messages);
    free(users);
    endwin();
}

void switch_window() {
    active_window = (active_window + 1) % 2;
}

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

    init_ncurses();
    create_windows();
    display_messages();
    display_users();
    display_input();

    pthread_t input_thread, server_thread;

    if (pthread_create(&input_thread, NULL, handle_events, NULL) != 0) {
        perror("Failed to create input thread");
        endwin();
        return EXIT_FAILURE;
    }
    if (pthread_create(&server_thread, NULL, handle_input, NULL) != 0) {
        perror("Failed to create server thread");
        endwin();
        return EXIT_FAILURE;
    }

    pthread_join(input_thread, NULL);
    pthread_cancel(server_thread);

    clean_up();
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

    free(messages);
    free(users);
    endwin();
    exit(0);
}

void handle_sigint(int sig) {
    cleanup_and_exit();
}

void* handle_events(void*){
    Msgbuf msg;
    User user;
    Message message;

    while (1) {
        if (msgrcv(user_info.desc, &msg, sizeof(Msgbuf) - sizeof(long), SERVER, IPC_NOWAIT) >= 0) {
            switch (msg.msg_type) {
                case MSGINFO:
                    strcpy(message.username, msg.username);
                    strcpy(message.message, msg.message);
                    strcpy(message.datetime, msg.datetime);

                    add_message(message, 0);
                    display_messages();
                    break;
                case USERINFO:
                    strcpy(user.username, msg.username);
                    user.id = msg.id;
                    user.desc = msg.desc;

                    add_user(user);
                    break;
                case DISCONNECT:
                    remove_user(msg.id);
                    display_users();
                    break;
            }
        }
        sleep_for_milliseconds(200);
    }
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

void sleep_for_milliseconds(long milliseconds) {
    struct timeval tv;
    tv.tv_sec = milliseconds / 1000;
    tv.tv_usec = (milliseconds % 1000) * 1000;
    select(0, NULL, NULL, NULL, &tv);
}
