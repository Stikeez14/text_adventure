#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <stdbool.h>

#define MAX_LINE_LENGTH 1024

#define RESET "\033[0m"

#define RED "\033[31m"
#define CYAN "\033[36m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define DARK_GRAY "\x1b[90m"
#define ORANGE "\x1b[38;5;214m"
#define PURPLE "\033[35m"

#define BRIGHT_GREEN "\033[1;92m"
#define BRIGHT_RED "\x1b[38;5;196m"

#define MAX_INVENTORY_ITEMS 3

#define MAX_HEATH 115 

typedef struct {
    int row;
    int col;
} Point;

typedef struct Node {
    Point point;
    struct Node *next;
} Node;

typedef struct Queue {
    Node *front;
    Node *rear;
} Queue;

/*
    GLOBAL VARIABLES
*/
int player_h = 100;
char items[MAX_INVENTORY_ITEMS] = {'\0','\0','\0'};

char **arena_files = NULL;
int current_arena = 0;  // keeps track of the current arena level
int num_arenas;

bool is_paused = false; // used for the pause menu
bool played_tutorial = false;

int death_flag = 0;
int weapon_flag = 0;
int block_input = 0;

int exit_arena = 0; // counts how many enemies and exit keys are required to open the door

int row_dir[] = {-1, 1, 0, 0};
int col_dir[] = {0, 0, -1, 1};


void set_arena_files(char **files, int count) {
    if (arena_files != NULL) free(arena_files);  // free old arena files if exist

    arena_files = (char **)malloc(count * sizeof(char *)); // allocate memory dynamically based on the count of files
    for (int i = 0; i < count; i++) arena_files[i] = files[i];
    num_arenas = count;  // set the number of arenas
}

/*
    FUNCTION PROTOTYPES 
*/
void start_game(); // game loop

void get_arena_dimensions(const char *file_name, int *rows, int *cols); // determines the rows and cols of the arena
char** create_arena(int rows, int cols); // allocates memory for the arena
void free_arena(char **arena, int rows); // frees the allocated memory
void initialize_arena(char **arena, int rows, int cols, const char *filename); // initializes the arena from the file
void print_arena(char **arena, int rows, int cols); 
void initialize_game(char ***arena, int *rows, int *cols, int *player_x, int *player_y); // dimensions + create + init + player position

void print_gui(char **arena, int rows, int cols); // gui + game window
void handle_player_health(int health); // prints player health
void handle_consumables(char items[]); // prints the invetory and items
void handle_tutorials(); // displays the right tutorial based on the arena
int process_player_inputs(int *player_x, int *player_y, char **arena, int rows, int cols); // takes keyboard inputs
int is_inventory_full(char items[]); 
void handle_inventory_slot(char *item_slot); 
void handle_arena_exit(char **arena, int rows, int cols); // unlocks the door after collecting the key + eliminating all threats

void reset_current_arena(char ***arena, int *rows, int *cols, int *player_x, int *player_y);
void reset_flags(int *spike_flag, int *exit_game, int *death_flag, int *weapon_flag,
                int *over_health_consumable, int *over_attack_consumable, int *over_info, int *over_small_hole);

Queue* create_queue();
void enqueue(Queue *queue, Point point);
Point dequeue(Queue *queue);
int is_empty(Queue *queue);
void fighters_bfs(char **arena, int rows, int cols, int player_x, int player_y, int dist[rows][cols]);
void move_fighters(char **arena, int rows, int cols, int player_x, int player_y);

void display_main_menu();
int display_pause_menu();
void display_tutorial_movement();
void display_tutorial0();
void display_tutorial1();
void display_tutorial_health();
void display_tutorial2();
void display_tutorial_inventory();
void display_tutorial3();
void display_tutorial4();
void display_tutorial_fail();
void display_spike_death();
void display_hole_death();
void display_warrior_death();
void display_exit_message();
void display_win();
void ask_about_tutorial();

/* 
    RAW MODE FOR INPUT, CONSOLE CLEAR, TERMINAL CURSOR & UNEXPECTED EXITS
    (only for linux at the moment) 
*/
void enable_raw_mode() { 
    struct termios term;
    tcgetattr(STDIN_FILENO, &term);
    term.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &term);
}

void disable_raw_mode() { 
    struct termios term;
    tcgetattr(STDIN_FILENO, &term);
    term.c_lflag |= (ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &term);
}

void hide_cursor() { 
    printf("\033[?25l");
    fflush(stdout);
}

void show_cursor() {
    printf("\033[?25h");
    fflush(stdout);
}

void cleanup() {
    disable_raw_mode();
    show_cursor();  
}

void handle_sigint(int sig) {
    cleanup();  
    exit(EXIT_SUCCESS);
}

void clear_console() { // clears the console screan based on OS
    #ifdef _WIN32
        system("cls");
    #elif __unix__ || __APPLE__
        system("clear");
    #else
        printf("console clearing not supported on this platform.\n");
    #endif
}

/* 
    MAIN FUNCTION
*/
int main() {

    signal(SIGINT, handle_sigint);
    
    atexit(cleanup);

    enable_raw_mode();

    hide_cursor();
    
    display_main_menu();

    return 0;
}

/* 
    GAME LOOP FUNCTION
*/
void start_game() {
  
    int rows, cols;
    char **arena;
    int player_x, player_y; // coordinates for player

    initialize_game(&arena, &rows, &cols, &player_x, &player_y);

    if(!current_arena && played_tutorial) display_tutorial_movement();
    
    char input; // stores keyboard input
    
    // flags
    int spike_flag = 0; 
    int exit_game = 0;

    int over_health_consumable = 0; // flags for knowing when the player is on top of items
    int over_attack_consumable = 0;
    int over_info = 0;
    int over_small_hole = 0;

    while (!exit_game) { 

        exit_arena = 0;
        handle_arena_exit(arena, rows, cols); // unlock exit door if all threats are eliminated

        print_gui(arena, rows, cols); // game window + gui

        block_input = 0;
    
        /* UPDATE ARENA CONSUMABLES AND TRAPS */
        if(spike_flag) { // if the player is over a spike 'x' ~ after leaving that position the spike remains there
            arena[player_x][player_y] = 'x'; 
            spike_flag = 0;
        }
        else if (over_info) {
            arena[player_x][player_y] = '!';  // same as for the spike
            over_info = 0;
        }
        else if (over_small_hole) {
            arena[player_x][player_y] = 'o'; // same as for the spike and info
            over_small_hole = 0;
        }
        else if (over_attack_consumable) {    // if the player is over an attack increase consumable 
            arena[player_x][player_y] = '^';  // and the inventory is full, the item remains on the ground 
            over_attack_consumable = 0;
        }
        else if (over_health_consumable) {    // same as for the attack consumable
            arena[player_x][player_y] = '+';
            over_health_consumable = 0;
        }
        else arena[player_x][player_y] = ' '; // clear current player location 
    
        /* INPUT AND PLAYER INTERACTIONS */
        if (!death_flag) exit_game = process_player_inputs(&player_x, &player_y, arena, rows, cols);
        if (exit_game) {
            current_arena = 0;
            continue;   // if the user leaves the game, skip the rest of the loop
            }

        if (arena[player_x][player_y] == 'x') { // -30 health if the player is on top of a spike
            if (!block_input) player_h -= 30; 
            spike_flag = 1; 
        }

        if ((arena[player_x][player_y] == 'w' || death_flag) && !weapon_flag) { // if player position = w position & the player has no weapon,
            player_h -= 200; 
            death_flag = 1;                                                    // he dies, oth the warrior dies
        }

        if (player_h <= 0 && (strstr(arena_files[current_arena], "arena") || strstr(arena_files[current_arena], "test"))) { // if health reaches 0 ~ death flag and break the loop
            current_arena = 0;
            if (spike_flag) display_spike_death();
            else if (death_flag) display_warrior_death(); 
            reset_flags(&spike_flag, &exit_game, &death_flag, &weapon_flag, &over_health_consumable, &over_attack_consumable, &over_info, &over_small_hole);
            reset_current_arena(&arena, &rows, &cols, &player_x, &player_y);
            break; 
        }
        else if (player_h <= 0 && strstr(arena_files[current_arena], "tutorial")) {
            reset_flags(&spike_flag, &exit_game, &death_flag, &weapon_flag, &over_health_consumable, &over_attack_consumable, &over_info, &over_small_hole);
            reset_current_arena(&arena, &rows, &cols, &player_x, &player_y); // reset the tutorial
            display_tutorial_fail();
            continue; // skip this game loop iteration
        }

        if (arena[player_x][player_y] == 'O' && (strstr(arena_files[current_arena], "arena") || strstr(arena_files[current_arena], "test"))) { // fall in hole ~ instant death
            current_arena = 0;
            reset_flags(&spike_flag, &exit_game, &death_flag, &weapon_flag, &over_health_consumable, &over_attack_consumable, &over_info, &over_small_hole);
            reset_current_arena(&arena, &rows, &cols, &player_x, &player_y);
            display_hole_death();
            break; 
        }
        else if (arena[player_x][player_y] == 'O' && strstr(arena_files[current_arena], "tutorial")) {
            reset_flags(&spike_flag, &exit_game, &death_flag, &weapon_flag, &over_health_consumable, &over_attack_consumable, &over_info, &over_small_hole);
            reset_current_arena(&arena, &rows, &cols, &player_x, &player_y); // reset the tutorial
            display_tutorial_fail();
            continue; // skip this game loop iteration
        }

        if (arena[player_x][player_y] == 'o') { // touching a small hole ~ lose all your items
            over_small_hole = 1;
            for (int i = 0; i < MAX_INVENTORY_ITEMS; i++) items[i] = '\0';
        }
        
        if (arena[player_x][player_y] == '+') {
            if (player_h <= 15) player_h += 15; // auto use the health consumable if health is lower than 15
            else if (!is_inventory_full(items)) { // if the inventory is not full, add the item
                for (int i = 0; i < MAX_INVENTORY_ITEMS; i++) {
                    if (items[i] == '\0') {
                        items[i] = '+';
                        break;
                    }
                }
            } 
            else over_health_consumable = 1; // inventory is full ~ activate flag to let the item on the ground
        }

        if (arena[player_x][player_y] == '^') {
            if (!is_inventory_full(items)) { // if the inventory is not full, add the item
                for (int i = 0; i < MAX_INVENTORY_ITEMS; i++) {
                    if (items[i] == '\0') {
                        items[i] = '^';
                        break;
                    }
                }
            } 
            else over_attack_consumable = 1;  // inventory is full ~ activate flag to let the item on the ground
        }

        if (arena[player_x][player_y] == 'k' || arena[player_x][player_y] == 'K') {
            for (int i = 0; i < rows; i++) { // change 'd' to ' ' after the key is picked
                for (int j = 0; j < cols; j++) if (arena[i][j] == 'd') arena[i][j] = ' ';
            }
        }

        if (arena[player_x][player_y] == '!') {
            over_info = 1;
            if (!block_input) handle_tutorials(); // print tutorial message based on the current tutorial arena
        }

        if (!block_input) move_fighters(arena, rows, cols, player_x, player_y);
  
        /* LOAD NEXT ARENA */
        if (arena[player_x][player_y] == '#') {
            if (current_arena + 1 < num_arenas) { // check if next arena is valid
                current_arena++;  // move to the next arena

                if (current_arena == 3 && played_tutorial) display_tutorial_health(); // display health system tutorial;
                else if (current_arena == 4 && played_tutorial) display_tutorial_inventory(); // display items & inventory tutorial

                if(strstr(arena_files[current_arena], "tutorial") || strstr(arena_files[current_arena], "welcome")) {
                    player_h = 100;
                    for (int i = 0; i < MAX_INVENTORY_ITEMS; i++) items[i] = '\0';
                }

                free_arena(arena, rows); // current area freed
                initialize_game(&arena, &rows, &cols, &player_x, &player_y); // initialize the next arena
            } 
            else { // last arena
                current_arena = 0;
                display_win(); // win message after the last arena
                reset_flags(&spike_flag, &exit_game, &death_flag, &weapon_flag, &over_health_consumable, &over_attack_consumable, &over_info, &over_small_hole);
                player_h = 100;
                for (int i = 0; i < MAX_INVENTORY_ITEMS; i++) items[i] = '\0';
                break;
            }
        }

        arena[player_x][player_y] = 'p'; // update player location on the arena
    }

    free_arena(arena, rows);
    clear_console();
}

/*
    ARENA FUNCTIONS
*/
void get_arena_dimensions(const char *file_name, int *rows, int *cols) { 
    FILE *fp = fopen(file_name, "r");
    if (!fp) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    char line[MAX_LINE_LENGTH];
    *rows = 0;
    *cols = 0;

    while (fgets(line, sizeof(line), fp)) { // read through each line
        (*rows)++;
        int length = strcspn(line, "\n"); // get line length ~ exluding '\n'
        if (length > *cols) {
            *cols = length;
        }
    }

    fclose(fp);
}

char** create_arena(int rows, int cols) { 
    char **arena = (char **)malloc(rows * sizeof(char *));
    if (arena == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < rows; i++) { // allocate memory for each row
        arena[i] = (char *)malloc(cols * sizeof(char));
        if (arena[i] == NULL) {
            perror("malloc");
            for (int j = 0; j < i; j++) free(arena[j]); // free previously allocated memory in case of error
            free(arena);
            exit(EXIT_FAILURE);
        }
    }
    return arena;
}

void free_arena(char **arena, int rows) {
    for (int i = 0; i < rows; i++) free(arena[i]);
    free(arena);
}

void initialize_arena(char **arena, int rows, int cols, const char *filename) { 
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }
    
    char line[MAX_LINE_LENGTH];
    for (int i = 0; i < rows; i++) {
        if (fgets(line, sizeof(line), file)) {
            int length = strcspn(line, "\n");
            for (int j = 0; j < cols; j++) {
                if (j < length) {
                    arena[i][j] = line[j];
                } else {
                    arena[i][j] = ' ';  // fill with spaces if line is shorter than expected
                }
            }
        } else {
            // fill the rest of the arena with spaces if file has fewer lines
            for (int j = 0; j < cols; j++) {
                arena[i][j] = ' ';
            }
        }
    }

    fclose(file);
}

void print_arena(char **arena, int rows, int cols) { 
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            if (arena[i][j] == 'w') printf("%s%c %s", BRIGHT_RED, arena[i][j], RESET); // enemies ~ BRIGHT_RED
            else if (arena[i][j] == 'x' || arena[i][j] == 'O' || arena[i][j] == 'o') printf("%s%c %s", RED, arena[i][j], RESET); // traps ~ RED)
            else if (arena[i][j] == '#' || arena[i][j] == 'K' || arena[i][j] == 'k' || arena[i][j] == '!' || arena[i][j] == '~' || 
                arena[i][j] == '<' || arena[i][j] == '>') printf("%s%c %s", YELLOW, arena[i][j], RESET); // exit ~ YELLOW
            else if (arena[i][j] == '+' || arena[i][j] == '^') printf("%s%c %s", GREEN, arena[i][j], RESET); // consumables ~ GREEN
            else if (arena[i][j] == 'p' || arena[i][j] == '0' || arena[i][j] == '1' || arena[i][j] == '2' || 
                arena[i][j] == '3' || arena[i][j] == '4' || arena[i][j] == '5' || arena[i][j] == '6' || arena[i][j] == '7' || 
                arena[i][j] == '8' || arena[i][j] == '9' || arena[i][j] == 'A' || arena[i][j] == 'R' || arena[i][j] == 'E' || 
                arena[i][j] == 'N' || arena[i][j] == 'T' || arena[i][j] == 'U' || arena[i][j] == 'R' || arena[i][j] == 'I' || 
                arena[i][j] == 'L' || arena[i][j] == 'W' || arena[i][j] == 'H' || arena[i][j] == 'V' || arena[i][j] == 'F' ||
                arena[i][j] == 'C' || arena[i][j] == 'M' ) printf("%s%c %s", CYAN, arena[i][j], RESET); // player ~ CYAN
            else if (arena[i][j] == 'D' || arena[i][j] == 'd') printf("%s%c %s", DARK_GRAY, arena[i][j], RESET); // doors ~ DARK_GRAY
            else printf("%c ", arena[i][j]); // rest of elements ~ REGULAR
        }
        printf("\n");
    }
}

void initialize_game(char ***arena, int *rows, int *cols, int *player_x, int *player_y) {
    
    get_arena_dimensions(arena_files[current_arena], rows, cols);
    *arena = create_arena(*rows, *cols); 
    initialize_arena(*arena, *rows, *cols, arena_files[current_arena]);

    for (int i = 0; i < *rows; i++) { // find the initial player position
        for (int j = 0; j < *cols; j++) {
            if ((*arena)[i][j] == 'p') {
                *player_x = i;
                *player_y = j;
                return;
            }
        }
    }
}

/*
    HANDLE FUNCTIONS
*/
void print_gui(char **arena, int rows, int cols) {
    clear_console(); 
    printf("= = = = = = = = = = = =\n|"); printf(" %s  _TEXT_ADVENTURE_  %s", ORANGE, RESET); printf("|\n"); 
    print_arena(arena, rows, cols);    
      
    if (played_tutorial) {
        if (current_arena == 3) {  
            handle_player_health(player_h); 
            printf("\n= = = =\n");
        }
        else if (current_arena > 3){ 
            handle_player_health(player_h);
            handle_consumables(items); 
        }  
    }
    else {
        if (current_arena > 0) {
            handle_player_health(player_h);
            handle_consumables(items); 
        }
    } 
}

void handle_player_health(int health) {
    if (health >= 100) { // print player health with green for [100,+]
        printf("|"); printf("%sH%s", GREEN, RESET); printf(":");
        printf("%s%d%s", GREEN, health, RESET); printf("|");
    }
    else if (health < 100 && health >= 75) { // // print player health with green [75,100]
        printf("|"); printf("%sH%s", GREEN, RESET); printf(":");
        printf("%s0%d%s", GREEN, health, RESET); printf("|");
    }
    else if (health < 75 && health >= 50) { // print player health with yellow for [50,75)
        printf("|"); printf("%sH%s", YELLOW, RESET); printf(":");
        printf("%s0%d%s", YELLOW, health, RESET); printf("|");
    }
    else if (health < 50 && health >= 25) { // print player health with orange for [25,50)
        printf("|"); printf("%sH%s", ORANGE, RESET); printf(":");
        printf("%s0%d%s", ORANGE, health, RESET); printf("|");
    }
    else if (health < 25 && health >= 10) { // print player health with red for [10,25)
        printf("|"); printf("%sH%s", BRIGHT_RED, RESET); printf(":");
        printf("%s0%d%s", BRIGHT_RED, health, RESET); printf("|");
    }
    else if (health < 10 && health >= 1) { // print player health with red for [1,10)
        printf("|"); printf("%sH%s", BRIGHT_RED, RESET); printf(":");
        printf("%s00%d%s", BRIGHT_RED, health, RESET); printf("|");
    }
    else { // print player health with red for (-,0]
        printf("|"); printf("%sH%s", BRIGHT_RED, RESET); printf(":");
        printf("%s000%s", BRIGHT_RED, RESET); printf("|");
    }
}

void handle_consumables(char items[]) { 
    for(int i = 0; i < MAX_INVENTORY_ITEMS; i++) {
        if (items[i] != '\0'){ 
            printf("%s[%s", ORANGE, RESET); printf("%s%c%s", GREEN, items[i], RESET); printf("%s]%s", ORANGE, RESET);
        }  
        else printf("%s[ ]%s", ORANGE, RESET);  
       
        if (i < MAX_INVENTORY_ITEMS) {
            printf("|");  
        }
    }
    printf("\n= = = = = = = = = =\n");
}

void handle_tutorials() { 
    char *arr = arena_files[current_arena];
    char last_char = arr[strlen(arr)-5];
            
    if(strstr(arena_files[current_arena],"tutorial")){
            
        switch(last_char){
            case '0': // tutorial about keys & doors
                display_tutorial0();
                break;
            case '1': // tutorial about special keys and exit doors unlocking
                display_tutorial1();
                break;
            case '2': // tutorial about spikes
                display_tutorial2();
                break;
            case '3': // tutorial about health increase
                display_tutorial3();
                break;
            case '4': // tutorial about holes
                display_tutorial4();
                break;
            default: break;
        }
    }
}

int process_player_inputs(int *player_x, int *player_y, char **arena, int rows, int cols) {
    char input = getchar();

    if (input == '\t') { // if tab is pressed
        is_paused = true;
        int pause = display_pause_menu(); // call the pause menu function
        if (pause) return 1; // if the user chooses to exit the game
        else block_input = 1; // block input in case he returns to the game
    }
    else if (input == '\x1b') {  // handling arrow keys
        input = getchar();
        if(input =='['){
            input = getchar();
            if(input == 'D' || input == 'A' || input == 'B' || input == 'C') {
                block_input = 1;
                return 0;
            }
        }
    }
  
    switch (input) {
        case 'w': case 'W': // move up
            if (*player_x > 1 && arena[*player_x - 1][*player_y] != '=' && arena[*player_x - 1][*player_y] != '|' && arena[*player_x - 1][*player_y] != 'D' && arena[*player_x - 1][*player_y] != 'd') 
                (*player_x)--;
            break;
        case 's': case 'S': // move down
            if (*player_x < rows - 2 && arena[*player_x + 1][*player_y] != '=' && arena[*player_x + 1][*player_y] != '|' && arena[*player_x + 1][*player_y] != 'D' && arena[*player_x + 1][*player_y] != 'd') 
                (*player_x)++;
            break;
        case 'a': case 'A': // move left
            if (*player_y > 1 && arena[*player_x][*player_y - 1] != '=' && arena[*player_x][*player_y - 1] != '|' && arena[*player_x][*player_y - 1] != 'D' && arena[*player_x][*player_y - 1] != 'd') 
                (*player_y)--;
            break;
        case 'd': case 'D': // move right
            if (*player_y < cols - 2 && arena[*player_x][*player_y + 1] != '=' && arena[*player_x][*player_y + 1] != '|' && arena[*player_x][*player_y + 1] != 'D' && arena[*player_x][*player_y + 1] != 'd') 
                (*player_y)++;
            break;
        case '1': // inventory slot 1
            handle_inventory_slot(&items[0]);
            block_input = 1;
            break;
        case '2': // inventory slot 2
            handle_inventory_slot(&items[1]);
            block_input = 1;
            break;
        case '3': // inventory slot 3
            handle_inventory_slot(&items[2]);
            block_input = 1;
            break;
        default:
            block_input = 1;
            break;
    }
    
    return 0; 
}

int is_inventory_full(char items[]) { 
    for (int i = 0; i < MAX_INVENTORY_ITEMS; i++) {
        if (items[i] == '\0') {
            return 0; // inventory is not full
        }
    }
    return 1; // inventory is full
}

void handle_inventory_slot(char *item_slot) {
    if(*item_slot == '+') {
        player_h += 15;
        if (player_h > MAX_HEATH) player_h = 115;
        *item_slot = '\0';
    }
    else if(*item_slot == '^') {
        weapon_flag = 1;
        *item_slot = '\0';
    }
}

void handle_arena_exit(char **arena, int rows, int cols) {
    for (int i = 0; i < rows; i++) { 
        for (int j = 0; j < cols; j++) if (arena[i][j] == 'K' || arena[i][j] == 'w') exit_arena++; 
    }

    if (!exit_arena) {
        for (int i = 0; i < rows; i++) { // change 'D' to '#' 
            for (int j = 0; j < cols; j++)  if (arena[i][j] == 'D') arena[i][j] = '#';
        }
    }
}
/*
    RESET FUNCTIONS
*/
void reset_current_arena(char ***arena, int *rows, int *cols, int *player_x, int *player_y) {
    player_h = 100;
    for (int i = 0; i < MAX_INVENTORY_ITEMS; i++) items[i] = '\0';

    free_arena(*arena, *rows); 
    initialize_game(arena, rows, cols, player_x, player_y);
}

void reset_flags(int *spike_flag, int *exit_game, int *death_flag, int *weapon_flag,
                int *over_health_consumable, int *over_attack_consumable, int *over_info, int *over_small_hole) {
    *spike_flag = 0;
    *exit_game = 0;
    *death_flag = 0;
    *weapon_flag = 0;
    *over_health_consumable = 0;
    *over_attack_consumable = 0;
    *over_info = 0;
    *over_small_hole = 0;
}

/*
    PATH FINDING
*/
Queue* create_queue() {
    Queue *queue = (Queue*)malloc(sizeof(Queue));
    queue->front = queue->rear = NULL;
    return queue;
}

void enqueue(Queue *queue, Point point) {
    Node *new_node = (Node*)malloc(sizeof(Node));
    new_node->point = point;
    new_node->next = NULL;
    if (queue->rear == NULL) {
        queue->front = queue->rear = new_node;
        return;
    }
    queue->rear->next = new_node;
    queue->rear = new_node;
}

Point dequeue(Queue *queue) {
    if (queue->front == NULL) {
        exit(EXIT_FAILURE);
    }
    Node *temp = queue->front;
    Point point = temp->point;
    queue->front = queue->front->next;
    if (queue->front == NULL) {
        queue->rear = NULL;
    }
    free(temp);
    return point;
}

int is_empty(Queue *queue) {
    return queue->front == NULL;
}

void fighters_bfs(char **arena, int rows, int cols, int player_x, int player_y, int dist[rows][cols]) {
    int visited[rows][cols];

    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            visited[i][j] = 0;
            dist[i][j] = -1;  
        }
    }

    Queue *queue = create_queue();
    enqueue(queue, (Point){player_x, player_y});
    visited[player_x][player_y] = 1;
    dist[player_x][player_y] = 0;

    while (!is_empty(queue)) { // bfs
        Point current = dequeue(queue);

        for (int i = 0; i < 4; i++) {  // explore the 4 directions 
            int new_row = current.row + row_dir[i];
            int new_col = current.col + col_dir[i];

            // check if new position is within bounds, walkable, and not visited
            if (new_row >= 0 && new_row < rows && new_col >= 0 && new_col < cols &&
                arena[new_row][new_col] != '|' && arena[new_row][new_col] != '=' && arena[new_row][new_col] != 'x' 
                && arena[new_row][new_col] != 'O' && arena[new_row][new_col] != 'D' && arena[new_row][new_col] != 'd' &&
                arena[new_row][new_col] != '#' && !visited[new_row][new_col]) {

                enqueue(queue, (Point){new_row, new_col});
                visited[new_row][new_col] = 1;
                dist[new_row][new_col] = dist[current.row][current.col] + 1;  // increment dist
            }
        }
    }

    free(queue);  
}

void move_fighters(char **arena, int rows, int cols, int player_x, int player_y) {
    int dist[rows][cols];

    fighters_bfs(arena, rows, cols, player_x, player_y, dist); // calculate distances from the player for each warrior

    char new_arena[rows][cols];

    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) new_arena[i][j] = arena[i][j];
    }

    int moved = 0;

    for (int i = 0; i < rows; i++) { // calculate where 'w' go
        for (int j = 0; j < cols; j++) {
            if (arena[i][j] == 'w') {
                int min_dist = dist[i][j];
                int next_row = i, next_col = j;
                int found_better_move = 0;

                for (int d = 0; d < 4; d++) { // check all 4 possible directions
                    int new_row = i + row_dir[d];
                    int new_col = j + col_dir[d];

                    if (new_row >= 0 && new_row < rows && new_col >= 0 && new_col < cols &&
                        dist[new_row][new_col] != -1 && dist[new_row][new_col] < min_dist &&
                        (arena[new_row][new_col] == ' ' || arena[new_row][new_col] == 'o' || arena[new_row][new_col] == '+' || 
                        arena[new_row][new_col] == '^' || arena[new_row][new_col] == 'p')) {

                        min_dist = dist[new_row][new_col];
                        next_row = new_row;
                        next_col = new_col;
                        found_better_move = 1;
                    }
                }

                if (found_better_move) {

                    if (next_row == player_x && next_col == player_y && !weapon_flag) {
                        death_flag = 1;
                        return;  // exit when reaching the player
                    }

                    if (new_arena[next_row][next_col] == ' ' || new_arena[next_row][next_col] == 'o' || new_arena[next_row][next_col] == '+' ||
                     new_arena[next_row][next_col] == '^' || arena[next_row][next_col] == 'p') {
                        new_arena[next_row][next_col] = 'w';  
                        new_arena[i][j] = ' ';  
                        moved = 1;
                    }
                }
            }
        }
    }

    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) arena[i][j] = new_arena[i][j];    
    }
}

/*
    MENUS & MESSAGES
*/
void display_main_menu() {
    while(1) {
        clear_console();
        printf("= = = = = = = = = = = =\n|"); 
        printf(" %s  _TEXT_ADVENTURE_  %s", ORANGE, RESET); printf("|\n");
        printf("= = = = = = = = = = = =\n"); 
        printf("|"); printf(" %sM %s", ORANGE, RESET); printf("|                 |\n");
        printf("|"); printf(" %sA %s", ORANGE, RESET); printf("| Hello, User!    |\n"); 
        printf("|"); printf(" %sI %s", ORANGE, RESET); printf("|                 |\n"); 
        printf("|"); printf(" %sN %s", ORANGE, RESET); printf("| 1: New Game     |\n");
        printf("|"); printf(" %s- %s", ORANGE, RESET); printf("| 2: Exit Game    |\n");
        printf("|"); printf(" %sM %s", ORANGE, RESET); printf("|                 |\n");
        printf("|"); printf(" %sE %s", ORANGE, RESET); printf("|                 |\n");
        printf("|"); printf(" %sN %s", ORANGE, RESET); printf("|                 |\n");
        printf("|"); printf(" %sU %s", ORANGE, RESET); printf("|                 |\n"); 
        printf("= = = = = = = = = = = =\n");

        char input;
        input = getchar();
        switch (input) {
            case '1': 
                ask_about_tutorial();
                char ask = getchar();
                if (ask == 'y' || ask == 'Y') {
                    played_tutorial = true;
                    clear_console();
                    char *tutorial_files[] = { "welcome.txt", "tutorial0.txt", "tutorial1.txt", "tutorial2.txt", "tutorial3.txt", "tutorial4.txt",
                        "arena0.txt", "arena1.txt", "arena2.txt"};
                    int count = sizeof(tutorial_files) / sizeof(tutorial_files[0]);
                    set_arena_files(tutorial_files, count);
                    start_game();
                    break;
                }
                else if (ask == 'n' || ask == 'N') {
                    played_tutorial = false;
                    clear_console();
                    char *non_tutorial_files[] = { "welcome.txt", "arena0.txt", "arena1.txt", "arena2.txt" };
                    int count = sizeof(non_tutorial_files) / sizeof(non_tutorial_files[0]);
                    set_arena_files(non_tutorial_files, count);
                    start_game();
                    break;
                }
                break;
            case '2': 
                display_exit_message();
                char ext = getchar();
                if (ext == 'y' || ext == 'Y') {
                    clear_console();
                    exit(EXIT_SUCCESS);
                }
                break;
            default: break;
        }
    }
}
 
int display_pause_menu() {
    while(1) {
        clear_console();
        printf("= = = = = = = = = = = =\n|"); 
        printf(" %s  _TEXT_ADVENTURE_  %s", ORANGE, RESET); printf("|\n");
        printf("= = = = = = = = = = = =\n"); 
        printf("|"); printf(" %sP %s", ORANGE, RESET); printf("| ___             |\n");
        printf("|"); printf(" %sA %s", ORANGE, RESET); printf("|                 |\n"); 
        printf("|"); printf(" %sU %s", ORANGE, RESET); printf("| Tab: Return to  |\n"); 
        printf("|"); printf(" %sS %s", ORANGE, RESET); printf("|      the arena  |\n");
        printf("|"); printf(" %sE %s", ORANGE, RESET); printf("|                 |\n");
        printf("|"); printf(" %s- %s", ORANGE, RESET); printf("| Esc: Go to the  |\n");
        printf("|"); printf(" %sM %s", ORANGE, RESET); printf("|      main menu  |\n");
        printf("|"); printf(" %sE %s", ORANGE, RESET); printf("| ___             |\n");
        printf("|"); printf(" %sN %s", ORANGE, RESET); printf("|                 |\n"); 
        printf("|"); printf(" %sU %s", ORANGE, RESET); printf("|                 |\n"); 
        printf("= = = = = = = = = = = =\n");

        char input;
        input = getchar();
        switch (input) {
            case '\t': 
                clear_console();
                is_paused = false;
                return 0;
            case '\x1B': 
                display_exit_message();
                char ext = getchar();
                if (ext == 'y' || ext == 'Y') {
                    clear_console();
                    return 1;
                }
                break;
            default: break;
        }
    }
}

void display_tutorial_movement() { // tutorial message for movement
    clear_console(); 
    printf("= = = = = = = = = = = =\n|"); 
    printf(" %s  _TEXT_ADVENTURE_  %s", ORANGE, RESET); printf("|\n");
    printf("= = = = = = = = = = = =\n"); 
    printf("|"); printf(" %sW %s", CYAN, RESET); printf("|                 |\n");
    printf("|"); printf(" %sE %s", CYAN, RESET); printf("| Use "); printf("%swasd%s", GREEN, RESET); printf(" (or    |\n");
    printf("|"); printf(" %sL %s", CYAN, RESET); printf("| uppercase "); printf("%sWASD%s", GREEN, RESET); printf(") |\n");
    printf("|"); printf(" %sC %s", CYAN, RESET); printf("| to move the     |\n"); 
    printf("|"); printf(" %s0 %s", CYAN, RESET); printf("| character!      |\n");
    printf("|"); printf(" %sM %s", CYAN, RESET); printf("|                 |\n"); 
    printf("|"); printf(" %sE %s", CYAN, RESET); printf("| Press any key   |\n");
    printf("|"); printf(" %s! %s", YELLOW, RESET); printf("| to continue ... |\n");
    printf("| = |                 |\n"); printf("= = = = = = = = = = = =\n");
    getchar(); 
    clear_console();
}

void display_tutorial0() { // tutorial message for the first tutorial arena
    clear_console(); 
    printf("= = = = = = = = = = = =\n|"); 
    printf(" %s  _TEXT_ADVENTURE_  %s", ORANGE, RESET); printf("|\n");
    printf("= = = = = = = = = = = =\n"); 
    printf("|"); printf(" %sT %s", CYAN, RESET); printf("|                 |\n");
    printf("|"); printf(" %sU %s", CYAN, RESET); printf("| Sometimes paths |\n"); 
    printf("|"); printf(" %sT %s", CYAN, RESET); printf("| are blocked by  |\n"); 
    printf("|"); printf(" %s0 %s", CYAN, RESET); printf("| doors ("); printf("%sd%s", DARK_GRAY, RESET); printf(").      |\n");
    printf("|"); printf(" %sR %s", CYAN, RESET); printf("|                 |\n");
    printf("|"); printf(" %sI %s", CYAN, RESET); printf("| Collect the key |\n");
    printf("|"); printf(" %sA %s", CYAN, RESET); printf("| ("); printf("%sk%s", YELLOW, RESET); printf(") to unlock   |\n");
    printf("|"); printf(" %sL %s", CYAN, RESET); printf("| them!           |\n");
    printf("| = |                 |\n"); printf("| = | Press any key   |\n");
    printf("| = | to continue ... |\n"); printf("| = |                 |\n"); 
    printf("= = = = = = = = = = = =\n");
    getchar(); 
    clear_console();
}

void display_tutorial1() { // tutorial message for the second tutorial arena
    clear_console(); 
    printf("= = = = = = = = = = = =\n|"); 
    printf(" %s  _TEXT_ADVENTURE_  %s", ORANGE, RESET); printf("|\n");
    printf("= = = = = = = = = = = =\n"); 
    printf("|"); printf(" %sT %s", CYAN, RESET); printf("|                 |\n");
    printf("|"); printf(" %sU %s", CYAN, RESET); printf("| If the arena    |\n"); 
    printf("|"); printf(" %sT %s", CYAN, RESET); printf("| exit door ("); printf("%s#%s", YELLOW, RESET); printf(")   |\n");
    printf("|"); printf(" %s0 %s", CYAN, RESET); printf("| is locked ("); printf("%sD%s", DARK_GRAY, RESET); printf("),  |\n");
    printf("|"); printf(" %sR %s", CYAN, RESET); printf("| collect the end |\n");
    printf("|"); printf(" %sI %s", CYAN, RESET); printf("| key ("); printf("%sK%s", YELLOW, RESET); printf(").        |\n");
    printf("|"); printf(" %sA %s", CYAN, RESET); printf("|                 |\n");
    printf("|"); printf(" %sL %s", CYAN, RESET); printf("| This special    |\n");
    printf("| = | key can open    |\n"); printf("| = | both exit and   |\n"); 
    printf("| = | path doors ("); printf("%sd%s", DARK_GRAY, RESET); printf("). |\n"); printf("| = |                 |\n");
    printf("| = | Press any key   |\n"); printf("| = | to continue ... |\n"); 
    printf("| = |                 |\n"); printf("= = = = = = = = = = = =\n");
    getchar(); 
    clear_console();
}

void display_tutorial_health() { // tutorial message for the health system
    clear_console(); 
    printf("= = = = = = = = = = = =\n|"); 
    printf(" %s  _TEXT_ADVENTURE_  %s", ORANGE, RESET); printf("|\n");
    printf("= = = = = = = = = = = =\n"); 
    printf("|"); printf(" %sT %s", CYAN, RESET); printf("|                 |\n");
    printf("|"); printf(" %sU %s", CYAN, RESET); printf("| "); printf("%sHEALTH SYSTEM%s", YELLOW, RESET); printf("   |\n");
    printf("|"); printf(" %sT %s", CYAN, RESET); printf("|                 |\n"); 
    printf("|"); printf(" %s0 %s", CYAN, RESET); printf("|  = = = =        |\n");
    printf("|"); printf(" %sR %s", CYAN, RESET); printf("|  |"); printf("%sH:100%s", GREEN, RESET); printf("|        |\n");
    printf("|"); printf(" %sI %s", CYAN, RESET); printf("|  = = = =        |\n");
    printf("|"); printf(" %sA %s", CYAN, RESET); printf("|                 |\n");
    printf("|"); printf(" %sL %s", CYAN, RESET); printf("| Enemies and     |\n");
    printf("| = | traps reduce    |\n"); printf("| = | your health.    |\n");
    printf("| = |                 |\n"); printf("| = | If it hits "); printf("%s0%s", RED, RESET); printf(",   |\n");
    printf("| = | you'll "); printf("%sdie!%s", RED, RESET); printf("     |\n"); printf("| = |                 |\n");
    printf("| = | Press any key   |\n"); printf("| = | to continue ... |\n"); 
    printf("| = |                 |\n"); printf("= = = = = = = = = = = =\n");
    getchar(); 
    clear_console();
}

void display_tutorial2() { // tutorial message for the third tutorial arena
    clear_console(); 
    printf("= = = = = = = = = = = =\n|"); 
    printf(" %s  _TEXT_ADVENTURE_  %s", ORANGE, RESET); printf("|\n");
    printf("= = = = = = = = = = = =\n"); 
    printf("|"); printf(" %sT %s", CYAN, RESET); printf("|                 |\n");
    printf("|"); printf(" %sU %s", CYAN, RESET); printf("| If you step on  |\n"); 
    printf("|"); printf(" %sT %s", CYAN, RESET); printf("| spikes ("); printf("%sx%s", RED, RESET); printf("),     |\n");
    printf("|"); printf(" %s0 %s", CYAN, RESET); printf("| your health     |\n"); 
    printf("|"); printf(" %sR %s", CYAN, RESET); printf("| will decrease   |\n");
    printf("|"); printf(" %sI %s", CYAN, RESET); printf("| by "); printf("%s30%s", RED, RESET); printf(".          |\n");
    printf("|"); printf(" %sA %s", CYAN, RESET); printf("|                 |\n");
    printf("|"); printf(" %sL %s", CYAN, RESET); printf("| Items improve   |\n");
    printf("| = | your defense    |\n"); printf("| = | againts spikes! |\n"); 
    printf("| = |                 |\n"); printf("| = | Press any key   |\n"); 
    printf("| = | to continue ... |\n"); printf("| = |                 |\n"); 
    printf("= = = = = = = = = = = =\n");
    getchar(); 
    clear_console();
}

void display_tutorial_inventory() { // tutorial message for the inventory system
    clear_console(); 
    printf("= = = = = = = = = = = =\n|"); 
    printf(" %s  _TEXT_ADVENTURE_  %s", ORANGE, RESET); printf("|\n");
    printf("= = = = = = = = = = = =\n"); 
    printf("|"); printf(" %sT %s", CYAN, RESET); printf("|                 |\n");
    printf("|"); printf(" %sU %s", CYAN, RESET); printf("| "); printf("%sITEMS &        %s", YELLOW, RESET); printf(" |\n");
    printf("|"); printf(" %sT %s", CYAN, RESET); printf("| "); printf("%sINVENTORY      %s", YELLOW, RESET); printf(" |\n"); 
    printf("|"); printf(" %s0 %s", CYAN, RESET); printf("|                 |\n");
    printf("|"); printf(" %sR %s", CYAN, RESET); printf("|  = = = = = = =  |\n");
    printf("|"); printf(" %sI %s", CYAN, RESET); printf("|  |"); printf("%s[%s", ORANGE, RESET); printf("%s1%s", GREEN, RESET); 
        printf("%s]%s", ORANGE, RESET); printf("|"); printf("%s[%s", ORANGE, RESET); printf("%s2%s", GREEN, RESET); printf("%s]%s", ORANGE, RESET); 
        printf("|"); printf("%s[%s", ORANGE, RESET); printf("%s3%s", GREEN, RESET); printf("%s]%s", ORANGE, RESET);  printf("|  |\n");
    printf("|"); printf(" %sA %s", CYAN, RESET); printf("|  = = = = = = =  |\n");
    printf("|"); printf(" %sL %s", CYAN, RESET); printf("|                 |\n");
    printf("| = | Items help you  |\n"); printf("| = | get through the |\n");
    printf("| = | the arenas more |\n"); printf("| = | easily.         |\n");
    printf("| = |                 |\n"); printf("| = | Use them by     |\n"); 
    printf("| = | pressing "); printf("%s1%s", GREEN, RESET); printf(", "); printf("%s2%s", GREEN, RESET); printf("   |\n");
    printf("| = | or "); printf("%s3%s", GREEN, RESET); printf(" for their  |\n"); 
    printf("| = | corresponding   |\n"); printf("| = | inventory slot. |\n");
    printf("| = |                 |\n"); printf("| = | Press any key   |\n"); printf("| = | to continue ... |\n"); 
    printf("| = |                 |\n"); printf("= = = = = = = = = = = =\n");
    getchar(); 
    clear_console();
}

void display_tutorial3() { // tutorial message for the fourth tutorial arena
    clear_console(); 
    printf("= = = = = = = = = = = =\n|"); 
    printf(" %s  _TEXT_ADVENTURE_  %s", ORANGE, RESET); printf("|\n");
    printf("= = = = = = = = = = = =\n"); 
    printf("|"); printf(" %sT %s", CYAN, RESET); printf("|                 |\n");
    printf("|"); printf(" %sU %s", CYAN, RESET); printf("| Your health is  |\n"); 
    printf("|"); printf(" %sT %s", CYAN, RESET); printf("| low?            |\n");
    printf("|"); printf(" %s0 %s", CYAN, RESET); printf("|                 |\n"); 
    printf("|"); printf(" %sR %s", CYAN, RESET); printf("| Pick up health  |\n");
    printf("|"); printf(" %sI %s", CYAN, RESET); printf("| increase ("); printf("%s+%s", GREEN, RESET); printf(")    |\n");
    printf("|"); printf(" %sA %s", CYAN, RESET); printf("| and restore     |\n");
    printf("|"); printf(" %sL %s", CYAN, RESET); printf("| your health!    |\n");
    printf("| = |                 |\n"); printf("| = | Without any     |\n"); 
    printf("| = | upgrades, this  |\n"); printf("| = | consumable will |\n");
    printf("| = | give you "); printf("%s+15 H.%s", GREEN, RESET); printf(" |\n"); printf("| = |                 |\n");
    printf("| = | If "); printf("%sH > 15%s", GREEN, RESET); printf(", the  |\n"); printf("| = | item gets into  |\n");  
    printf("| = | the inventory!  |\n"); printf("| = |                 |\n"); 
    printf("| = | Press any key   |\n"); printf("| = | to continue ... |\n"); 
    printf("| = |                 |\n"); printf("= = = = = = = = = = = =\n");
    getchar(); 
    clear_console();
}

void display_tutorial4() { // tutorial message for the fifth tutorial arena
    clear_console(); 
    printf("= = = = = = = = = = = =\n|"); 
    printf(" %s  _TEXT_ADVENTURE_  %s", ORANGE, RESET); printf("|\n");
    printf("= = = = = = = = = = = =\n"); 
    printf("|"); printf(" %sT %s", CYAN, RESET); printf("|                 |\n");
    printf("|"); printf(" %sU %s", CYAN, RESET); printf("| Be carefull!!!  |\n"); 
    printf("|"); printf(" %sT %s", CYAN, RESET); printf("|                 |\n");
    printf("|"); printf(" %s0 %s", CYAN, RESET); printf("| Holes appeared  |\n"); 
    printf("|"); printf(" %sR %s", CYAN, RESET); printf("| out of nowhere. |\n");
    printf("|"); printf(" %sI %s", CYAN, RESET); printf("|                 |\n");
    printf("|"); printf(" %sA %s", CYAN, RESET); printf("| Falling into a  |\n");
    printf("|"); printf(" %sL %s", CYAN, RESET); printf("| a big hole ("); printf("%sO%s", RED, RESET); printf(")  |\n");
    printf("| = | means instant   |\n"); printf("| = | ");  printf("%sdeath!%s", RED, RESET); printf("          |\n");
    printf("| = |                 |\n"); printf("| = | Small holes ("); printf("%so%s", RED, RESET); printf(") |\n");
    printf("| = | will not give   |\n"); printf("| = | you any damage  |\n");
    printf("| = | "); printf("%sBUT%s", YELLOW, RESET); printf(" all your    |\n"); printf("| = | items will be   |\n");  
    printf("| = | "); printf("%slost%s", RED, RESET); printf(" :)         |\n"); printf("| = |                 |\n"); 
    printf("| = | Press any key   |\n"); printf("| = | to continue ... |\n"); 
    printf("| = |                 |\n"); printf("= = = = = = = = = = = =\n");
    getchar(); 
    clear_console();
}

void display_tutorial_fail() { // message displayed when the tutorial is failed 
    clear_console(); 
    printf("= = = = = = = = = = = =\n|"); 
    printf(" %s  _TEXT_ADVENTURE_  %s", ORANGE, RESET); printf("|\n");
    printf("= = = = = = = = = = = =\n"); 
    printf("|"); printf(" %sT %s", CYAN, RESET); printf("|                 |\n");
    printf("|"); printf(" %sU %s", CYAN, RESET); printf("| "); printf("%sYOU ARE DEAD!%s", BRIGHT_RED, RESET); printf("   |\n");
    printf("|"); printf(" %sT %s", CYAN, RESET); printf("|                 |\n");
    printf("|"); printf(" %s0 %s", CYAN, RESET); printf("| Press any key   |\n"); 
    printf("|"); printf(" %sR %s", CYAN, RESET); printf("| to restart this |\n");
    printf("|"); printf(" %sI %s", CYAN, RESET); printf("| tutorial arena  |\n");
    printf("|"); printf(" %sA %s", CYAN, RESET); printf("| ...             |\n");
    printf("|"); printf(" %sL %s", CYAN, RESET); printf("|                 |\n"); 
    printf("= = = = = = = = = = = =\n");
    getchar(); 
    clear_console();
}

void display_spike_death() { // death message for spikes
    clear_console(); 
    printf("= = = = = = = = = = = =\n|"); 
    printf(" %s  _TEXT_ADVENTURE_  %s", ORANGE, RESET); printf("|\n");
    printf("= = = = = = = = = = = =\n"); 
    printf("|"); printf(" %sG %s", RED, RESET); printf("|                 |\n");
    printf("|"); printf(" %sA %s", RED, RESET); printf("| "); printf("%sYOU ARE DEAD!%s", BRIGHT_RED, RESET); printf("   |\n");
    printf("|"); printf(" %sM %s", RED, RESET); printf("|                 |\n");
    printf("|"); printf(" %sE %s", RED, RESET); printf("| An ordinary     |\n"); 
    printf("|"); printf(" %s- %s", RED, RESET); printf("| spike took your |\n");
    printf("|"); printf(" %sO %s", RED, RESET); printf("| life :)         |\n");
    printf("|"); printf(" %sV %s", RED, RESET); printf("|                 |\n");
    printf("|"); printf(" %sE %s", RED, RESET); printf("| Press any key   |\n"); 
    printf("|"); printf(" %sR %s", RED, RESET); printf("| and return back |\n"); 
    printf("| = | to the main     |\n"); printf("| = | menu ...        |\n"); 
    printf("| = |                 |\n"); printf("= = = = = = = = = = = =\n");
    getchar(); 
    clear_console();
}

void display_hole_death() { // death message for holes
    clear_console(); 
    printf("= = = = = = = = = = = =\n|"); 
    printf(" %s  _TEXT_ADVENTURE_  %s", ORANGE, RESET); printf("|\n");
    printf("= = = = = = = = = = = =\n"); 
    printf("|"); printf(" %sG %s", RED, RESET); printf("|                 |\n");
    printf("|"); printf(" %sA %s", RED, RESET); printf("| "); printf("%sYOU ARE DEAD!%s", BRIGHT_RED, RESET); printf("   |\n");
    printf("|"); printf(" %sM %s", RED, RESET); printf("|                 |\n");
    printf("|"); printf(" %sE %s", RED, RESET); printf("| It looks like   |\n"); 
    printf("|"); printf(" %s- %s", RED, RESET); printf("| you fell into a |\n");
    printf("|"); printf(" %sO %s", RED, RESET); printf("| very big hole!  |\n");
    printf("|"); printf(" %sV %s", RED, RESET); printf("|                 |\n");
    printf("|"); printf(" %sE %s", RED, RESET); printf("| Press any key   |\n"); 
    printf("|"); printf(" %sR %s", RED, RESET); printf("| and return back |\n"); 
    printf("| = | to the main     |\n"); printf("| = | menu ...        |\n"); 
    printf("| = |                 |\n"); printf("= = = = = = = = = = = =\n");
    getchar(); 
    clear_console();
}

void display_warrior_death() { // death message for warriors
    clear_console(); 
    printf("= = = = = = = = = = = =\n|"); 
    printf(" %s  _TEXT_ADVENTURE_  %s", ORANGE, RESET); printf("|\n");
    printf("= = = = = = = = = = = =\n"); 
    printf("|"); printf(" %sG %s", RED, RESET); printf("|                 |\n");
    printf("|"); printf(" %sA %s", RED, RESET); printf("| "); printf("%sYOU ARE DEAD!%s", BRIGHT_RED, RESET); printf("   |\n");
    printf("|"); printf(" %sM %s", RED, RESET); printf("|                 |\n");
    printf("|"); printf(" %sE %s", RED, RESET); printf("| Warriors are    |\n"); 
    printf("|"); printf(" %s- %s", RED, RESET); printf("| hard to defeat  |\n");
    printf("|"); printf(" %sO %s", RED, RESET); printf("| if you are not  |\n");
    printf("|"); printf(" %sV %s", RED, RESET); printf("| skilled!        |\n");
    printf("|"); printf(" %sE %s", RED, RESET); printf("|                 |\n"); 
    printf("|"); printf(" %sR %s", RED, RESET); printf("| Press any key   |\n"); 
    printf("| = | and return back |\n"); printf("| = | to the main     |\n"); 
    printf("| = | menu ...        |\n"); printf("| = |                 |\n"); 
    printf("= = = = = = = = = = = =\n");
    getchar(); 
    clear_console();
}

void display_exit_message() { // message displayed when exiting the game
    clear_console(); 
    printf("= = = = = = = = = = = =\n|"); 
    printf(" %s  _TEXT_ADVENTURE_  %s", ORANGE, RESET); printf("|\n");
    printf("= = = = = = = = = = = =\n"); 
    printf("|"); printf(" %sM %s", ORANGE, RESET); printf("|                 |\n");
    printf("|"); printf(" %sA %s", ORANGE, RESET); printf("| Are you sure    |\n");
    printf("|"); printf(" %sI %s", ORANGE, RESET); printf("| about leaving   |\n");
    printf("|"); printf(" %sN %s", ORANGE, RESET); printf("| the game?  "); printf("%s:(%s", YELLOW, RESET); printf("   |\n");
    printf("|"); printf(" %s- %s", ORANGE, RESET); printf("|                 |\n");
    printf("|"); printf(" %sM %s", ORANGE, RESET); printf("| Press "); printf("%sY%s", RED, RESET); printf(" or "); printf("%sy%s", RED, RESET); printf(" to |\n");
    printf("|"); printf(" %sE %s", ORANGE, RESET); printf("| "); printf("%sexit%s", RED, RESET); printf(" the game.  |\n");
    printf("|"); printf(" %sN %s", ORANGE, RESET); printf("|                 |\n");
    printf("|"); printf(" %sU %s", ORANGE, RESET); printf("| Press any other |\n"); 
    printf("| = | key to ");  printf("%sstay%s", GREEN, RESET); printf(" ... |\n");
    printf("| = |                 |\n"); printf("= = = = = = = = = = = =\n");
}

void display_win() { // message displayed when finishing all arenas
    clear_console(); 
    printf("= = = = = = = = = = = =\n|"); 
    printf(" %s  _TEXT_ADVENTURE_  %s", ORANGE, RESET); printf("|\n");
    printf("= = = = = = = = = = = =\n"); 
    printf("|"); printf(" %sV %s", GREEN, RESET); printf("|                 |\n");
    printf("|"); printf(" %sI %s", GREEN, RESET); printf("| "); printf("%sCONGRATULATIONS%s", BRIGHT_GREEN, RESET); printf(" |\n");
    printf("|"); printf(" %sC %s", GREEN, RESET); printf("|                 |\n");
    printf("|"); printf(" %sT %s", GREEN, RESET); printf("| You completed   |\n"); 
    printf("|"); printf(" %sO %s", GREEN, RESET); printf("| all the arenas! |\n");
    printf("|"); printf(" %sR %s", GREEN, RESET); printf("|                 |\n");
    printf("|"); printf(" %sY %s", GREEN, RESET); printf("| Thank you for   |\n");
    printf("|"); printf(" %s! %s", YELLOW, RESET); printf("| playing "); printf("%sv.1.0%s", YELLOW, RESET); printf("   |\n"); 
    printf("| = |                 |\n"); printf("| = | Press any key   |\n"); 
    printf("| = | and return back |\n"); printf("| = | to the main     |\n"); 
    printf("| = | menu ...        |\n"); printf("| = |                 |\n"); 
    printf("= = = = = = = = = = = =\n");
    getchar(); 
    clear_console();
}

void ask_about_tutorial() { // message that asks the user if he wants to play the tutorial
    clear_console();
    printf("= = = = = = = = = = = =\n|"); 
    printf(" %s  _TEXT_ADVENTURE_  %s", ORANGE, RESET); printf("|\n");
    printf("= = = = = = = = = = = =\n"); 
    printf("|"); printf(" %sM %s", ORANGE, RESET); printf("|                 |\n");
    printf("|"); printf(" %sA %s", ORANGE, RESET); printf("| Do you want     |\n"); 
    printf("|"); printf(" %sI %s", ORANGE, RESET); printf("| to play the     |\n"); 
    printf("|"); printf(" %sN %s", ORANGE, RESET); printf("| tutorial?       |\n");
    printf("|"); printf(" %s- %s", ORANGE, RESET); printf("| ("); printf("%srecommended%s", GREEN, RESET); printf(")   |\n");
    printf("|"); printf(" %sM %s", ORANGE, RESET); printf("|                 |\n");
    printf("|"); printf(" %sE %s", ORANGE, RESET); printf("| Press "); printf("%sY%s", GREEN, RESET); printf(" or "); printf("%sy%s", GREEN, RESET); printf(" to |\n");
    printf("|"); printf(" %sN %s", ORANGE, RESET); printf("| for playing it. |\n");
    printf("|"); printf(" %sU %s", ORANGE, RESET); printf("|                 |\n"); 
    printf("| = | Press "); printf("%sN%s", YELLOW, RESET); printf(" or "); printf("%sn%s", YELLOW, RESET); printf("    |\n"); printf("| = | for starting    |\n"); 
    printf("| = | without it.     |\n"); printf("| = |                 |\n"); 
    printf("| = | Press any other |\n"); printf("| = | key to return   |\n"); 
    printf("| = | back ...        |\n"); printf("| = |                 |\n"); 
    printf("= = = = = = = = = = = =\n");
}


