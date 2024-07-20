#include "../include/messanger.h"

Message *messages;
int messages_size;

User *users;
int users_size;

char input_message[MAX_MSG_LEN];

WINDOW *msg_win, *user_win, *input_win;
WINDOW *msg_win_border, *user_win_border, *input_win_border;
int msg_win_height, msg_win_width;
int user_win_height, user_win_width;
int input_win_height, input_win_width;
int key_catcher[3];

int msg_scroll_pos;
int user_scroll_pos;
int active_window; // 0 - msg_win, 1 - user_win

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

void display_messages() {
    werase(msg_win);
    int start = messages_size - msg_scroll_pos - msg_win_height / 2;
    if (start < 0) start = 0;
    for (int i = start; i < start + msg_win_height / 2; i++) {
        if (i >= messages_size) break;

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
        if (i >= users_size) break;

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

void catch(const char ch) {
    key_catcher[0] = key_catcher [1];
    key_catcher[1] = key_catcher[2];
    key_catcher[2] = ch;
}

bool is_key_up(){
    return key_catcher[0] == 27 && key_catcher[1] == 91 && key_catcher[2] == 65;
}

bool is_key_down(){
    return key_catcher[0] == 27 && key_catcher[1] == 91 && key_catcher[2] == 66;
}

void read_message() {
    int ch;
    int input_index = strlen(input_message);

    while ((ch = wgetch(input_win)) != '\n') {
        catch(ch);

        if (ch == KEY_BACKSPACE || ch == 127 || ch == '\b') {
            if (input_index > 0) {
                input_message[--input_index] = '\0';
            }
        } else if (ch == KEY_RESIZE) {
            create_windows();
            display_messages();
            display_users();
            display_input();
        } else if (ch == '\t') {
            switch_window();
        } else if (is_key_up()) {
            input_index -= 2;
            input_message[input_index] = '\0';

            if (active_window == 0 && msg_scroll_pos < messages_size - msg_win_height / 2) {
                msg_scroll_pos++;
                display_messages();
            } else if (active_window == 1 && user_scroll_pos < users_size - user_win_height) {
                user_scroll_pos++;
                display_users();
            }
        } else if (is_key_down()) {
            input_index -= 2;
            input_message[input_index] = '\0';

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

void switch_window() {
    active_window = (active_window + 1) % 2;
}

void run_messanger(const char *username) {
    strcpy(input_message, "");
    signal(SIGINT, handle_sigint);
    key_catcher[0] = 0;
    key_catcher[1] = 0;
    key_catcher[2] = 0;

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
    strcpy(reg_message.username, username);
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

    cleanup_and_exit();
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
        sleep_for_milliseconds(10);
    }
}

void sleep_for_milliseconds(long milliseconds) {
    struct timeval tv;
    tv.tv_sec = milliseconds / 1000;
    tv.tv_usec = (milliseconds % 1000) * 1000;
    select(0, NULL, NULL, NULL, &tv);
}
