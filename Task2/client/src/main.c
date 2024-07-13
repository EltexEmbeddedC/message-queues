#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_NAME_LEN 32
#define MAX_MSG_LEN 512
#define MAX_DATE_LEN 64

typedef struct {
    char username[MAX_NAME_LEN];
    char message[MAX_MSG_LEN];
    char datetime[MAX_DATE_LEN];
} UserMessage;

typedef struct {
    char username[MAX_NAME_LEN];
    int id;
} User;

UserMessage *messages = NULL;
int message_count = 0;

User *users = NULL;
int user_count = 0;

char input_message[MAX_MSG_LEN] = "";
char current_user[MAX_NAME_LEN] = "";

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
void add_message(const char *username, const char *message);
void add_user(const char *username);
void display_messages();
void display_users();
void display_input();
void handle_input();
void clean_up();
void demo_data();
void ask_username();
void switch_window();

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

void add_message(const char *username, const char *message) {
    messages = realloc(messages, sizeof(UserMessage) * (message_count + 1));
    strncpy(messages[message_count].username, username, MAX_NAME_LEN);
    strncpy(messages[message_count].message, message, MAX_MSG_LEN);

    time_t now = time(NULL);
    strftime(messages[message_count].datetime, MAX_DATE_LEN, "%Y-%m-%d %H:%M:%S", localtime(&now));
    message_count++;
}

void add_user(const char *username) {
    users = realloc(users, sizeof(User) * (user_count + 1));
    strncpy(users[user_count].username, username, MAX_NAME_LEN);
    users[user_count].id = user_count + 1; // Simple ID assignment for demonstration
    user_count++;
}

void display_messages() {
    werase(msg_win);
    int start = message_count - msg_scroll_pos - msg_win_height / 2;
    if (start < 0) start = 0;
    for (int i = start; i < message_count; i++) {
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
    for (int i = start; i < user_count; i++) {
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

void handle_input() {
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
            if (active_window == 0 && msg_scroll_pos < message_count - msg_win_height / 2) {
                msg_scroll_pos++;
                display_messages();
            } else if (active_window == 1 && user_scroll_pos < user_count - user_win_height) {
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
        add_message(current_user, input_message);
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

void demo_data() {
    add_user("Alice");
    add_user("Bob");
    add_user("Charlie");
    add_message("Alice", "Hello, world!");
    add_message("Bob", "Hi, Alice!");
    add_message("Charlie", "Good morning!");
}

void ask_username() {
    echo();
    mvprintw(LINES / 2, (COLS - 20) / 2, "Enter your name: ");
    getnstr(current_user, MAX_NAME_LEN - 1);
    noecho();
}

void switch_window() {
    active_window = (active_window + 1) % 2;
}

int main() {
    init_ncurses();
    ask_username();
    create_windows();
    demo_data();
    display_messages();
    display_users();
    display_input();

    while (1) {
        handle_input();
    }

    clean_up();
    return 0;
}
