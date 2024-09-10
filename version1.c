#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#define MAX_LINE_LENGTH 1024

#define RESET "\033[0m"
#define RED "\033[31m"
#define CYAN "\033[36m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define DARK_GRAY "\x1b[90m"
#define BRIGHT_RED "\x1b[38;5;196m"

#define MAX_INVENTORY_ITEMS 3
 
/*
    GLOBAL VARIABLES
*/
int player_h = 100;

char items[MAX_INVENTORY_ITEMS] = {'\0','\0','\0'};

int weapon_flag = 0;

char *arena_files[] = {"welcome.txt", "tutorial0.txt", "tutorial1.txt", "tutorial2.txt", "tutorial3.txt", "tutorial4.txt",
"arena0.txt", "arena1.txt", "arena2.txt"};


int current_arena = 0;  // keeps track of the current arena level

int num_arenas = sizeof(arena_files) / sizeof(arena_files[0]);

/*
    FUNCTION PROTOTYPES
*/
void display_tutorial0();

void display_tutorial1();

void display_tutorial2();

void display_tutorial3();

void display_tutorial4();

/* 
    RAW MODE FOR INPUT, CONSOLE CLEAR, TERMINAL CURSOR & UNEXPECTED EXITS
*/
void enable_raw_mode() { /* only for linux */
    struct termios term;
    tcgetattr(STDIN_FILENO, &term);
    term.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &term);
}

void disable_raw_mode() { /* only for linux */
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
    ARENA FUNCTIONS
*/
void get_arena_dimensions(const char *file_name, int *rows, int *cols) { // determines the rows and cols of the arena
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

char** create_arena(int rows, int cols) { // allocates memory for the arena
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

void free_arena(char **arena, int rows) { // frees the allocated memory
    for (int i = 0; i < rows; i++) {
        free(arena[i]);
    }
    free(arena);
}

void initialize_arena(char **arena, int rows, int cols, const char *filename) {  // initializes the arena from the file
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

/*
    HANDLE FUNCTIONS
*/
void handle_player_health(int health) {
    if (health >= 100) { // print player health with green for [100,+]
        printf("|"); printf("%sH%s", GREEN, RESET); printf(":");
        printf("%s%d%s", GREEN, health, RESET); printf("|");
    }
    else if (health < 100 && health >= 70) { // // print player health with green [70,100]
        printf("|"); printf("%sH%s", GREEN, RESET); printf(":");
        printf("%s0%d%s", GREEN, health, RESET); printf("|");
    }
    else if (health < 70 && health >= 30) { // print player health with yellow for [30,70)
        printf("|"); printf("%sH%s", YELLOW, RESET); printf(":");
        printf("%s0%d%s", YELLOW, health, RESET); printf("|");
    }
    else if (health < 30 && health >= 10) { // print player health with red for [10,30)
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

void handle_consumables(char items[]){
    for(int i = 0; i < MAX_INVENTORY_ITEMS; i++) {
        if (items[i] != '\0'){ 
            printf("%s[%s", YELLOW, RESET); printf("%s%c%s", GREEN, items[i], RESET); printf("%s]%s", YELLOW, RESET);
        }  
        else printf("%s[ ]%s", YELLOW, RESET);  
       
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
            case '0':
                clear_console();
                display_tutorial0();
                getchar();
                clear_console();
                break;
            case '1':
                clear_console();
                display_tutorial1();
                getchar();
                clear_console();
                break;
            case '2':
                clear_console();
                display_tutorial2();
                getchar();
                clear_console();
                break;
            case '3':
                clear_console();
                display_tutorial3();
                getchar();
                clear_console();
                break;
            case '4':
                clear_console();
                display_tutorial4();
                getchar();
                clear_console();
                break;
            default: break;
        }
    }
}

int process_player_inputs(int *player_x, int *player_y, char **arena, int rows, int cols) {
    char input = getchar();

    if (input == '\x1b') {  
        input = getchar();
        if(input =='['){
            input = getchar();
            if(input == 'D' || input == 'A' || input == 'B' || input == 'C') return 0;
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
        case '\t': // exit by pressing tab
            return 1;
        case '1': // consumable 1
            if(items[0] == '+') {
                player_h += 15;
                items[0] = '\0';
                }
            else if(items[0] == '^') {
                weapon_flag = 1;
                items[0] = '\0';
            }
            break;
        case '2': // consumable 2
            if(items[1] == '+') {
                player_h += 15;
                items[1] = '\0';
              
                }
            else if(items[1] == '^') {
                weapon_flag = 1;
                items[1] = '\0';
            }
            break;
        case '3': // consumable 3
            if(items[2] == '+') {
                player_h += 15;
                items[2] = '\0';
                }
            else if(items[2] == '^') {
                weapon_flag = 1;
                items[2] = '\0';
            }
            break;
        default:
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

void initialize_game(const char *arena_file, char ***arena, int *rows, int *cols, int *player_x, int *player_y) {
    
    get_arena_dimensions(arena_file, rows, cols); // read the arena file and get he numbers of rows and cols
    
    *arena = create_arena(*rows, *cols); // allocate memory dinamically for the arena
    
    initialize_arena(*arena, *rows, *cols, arena_file);

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

int main() {

    signal(SIGINT, handle_sigint);
    
    atexit(cleanup);

    int rows, cols;

    const char *arena_file = arena_files[current_arena];

    char **arena;

    int player_x, player_y; // coordinates for player

    initialize_game(arena_file, &arena, &rows, &cols, &player_x, &player_y);

    enable_raw_mode();
    hide_cursor();

    char input; // stores keyboard input
    
    /* FLAGS */
    int death_flag = 0;
    int spike_flag = 0; 
    int exit_game = 0;
    int warrior_flag = 0;
    int next_arena = 0; 

    int over_health_consumable = 0; // flags for knowing when the player is on top of items
    int over_attack_consumable = 0;
    int over_info = 0;
    int over_small_hole = 0;

    while (!exit_game) { // game loop

        clear_console(); // clear console after each iteration
        printf("= = = = = = = = = = = =\n|"); printf(" %s  _TEXT_ADVENTURE_  %s", YELLOW, RESET); printf("|\n");
        print_arena(arena, rows, cols);

        handle_player_health(player_h);
        handle_consumables(items);
    
        if(player_h <= 0) { // if health reaches 0 ~ death flag and break the loop
            death_flag = 1;
            clear_console(); // clear console after death
            break; 
        }


        /* UPDATE ARENA CONSUMABLES AND TRAPS */
        if(spike_flag){ // if the player is over a spike 'x' ~ after leaving that position the spike remains there
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
        exit_game = process_player_inputs(&player_x, &player_y, arena, rows, cols);   

        if (arena[player_x][player_y] == 'x'){ // -30 health if the player is on top of a spike
            player_h -= 30; 
            spike_flag = 1; 
        }

        if (arena[player_x][player_y] == 'O'){ // fall in hole ~ instant death
            death_flag = 1;
            clear_console();
            break; 
        }

        if (arena[player_x][player_y] == 'o'){ // touching a small hole ~ lose all your items
            over_small_hole = 1;
            for (int i = 0; i < MAX_INVENTORY_ITEMS; i++) {
                items[i] = '\0';
            }
        }


        if (arena[player_x][player_y] == 'w' && !weapon_flag){  // if player position = w position & the player has no weapon,
            player_h -= 200;                                    // he dies, oth the warrior dies
            warrior_flag = 1; 
        }
        
        if (arena[player_x][player_y] == '+') {
            if (player_h <= 15) { // auto use the health consumable if health is lower than 15
                player_h += 15;
            } 
            else if (!is_inventory_full(items)) { // if the inventory is not full, add the item
                for (int i = 0; i < MAX_INVENTORY_ITEMS; i++) {
                    if (items[i] == '\0') {
                        items[i] = '+';
                        break;
                    }
                }
            } 
            else { // inventory is full ~ activate flag to let the item on the ground
                over_health_consumable = 1;
            }
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
            else { // inventory is full ~ activate flag to let the item on the ground
                over_attack_consumable = 1;
            }
        }

        if (arena[player_x][player_y] == '#') next_arena = 1; 

        if (arena[player_x][player_y] == 'K') {
            for (int i = 0; i < rows; i++) { // change 'D' to '#' and 'd' to ' ' after the key is picked
                for (int j = 0; j < cols; j++) {
                    if (arena[i][j] == 'D') arena[i][j] = '#';
                    else if (arena[i][j] == 'd') arena[i][j] = ' ';
                }
            }
        }

        if (arena[player_x][player_y] == 'k') {
            for (int i = 0; i < rows; i++) { // change 'd' to ' ' after the key is picked
                for (int j = 0; j < cols; j++) if (arena[i][j] == 'd') arena[i][j] = ' ';
            }
        }

        if (arena[player_x][player_y] == '!') {
            over_info = 1;
            handle_tutorials(); // print tutorial message based on the current tutorial arena
        }

        /* LOAD NEXT ARENA */
        if (next_arena){
            current_arena++;  // move to the next arena
            if (current_arena < num_arenas) {
            
                free_arena(arena, rows); // current area freed
                get_arena_dimensions(arena_files[current_arena], &rows, &cols); // get dimentions for the next one
                arena = create_arena(rows, cols); // dynamic allocate memory for next arena
                initialize_arena(arena, rows, cols, arena_files[current_arena]); // init next arena

                for (int i = 0; i < rows; i++) { // reset player position
                    for (int j = 0; j < cols; j++) {
                        if (arena[i][j] == 'p') {
                            player_x = i;
                            player_y = j;
                            break;
                        }
                    }
                }
                next_arena = 0;  
            } else {
                clear_console(); // clear console after the last arena
                break;
            }
        }

        arena[player_x][player_y] = 'p'; // update player location on the arena

        if(warrior_flag) arena[player_x][player_y] = 'w'; // if 'w' kills 'p' he overrides the player on the map
        else if (spike_flag && player_h <= 0) arena[player_x][player_y] = 'x'; // 'p' overrides a spike when he s on
                                                                               // as long as he s not dead
    }

    free_arena(arena, rows);

    clear_console();

    if(death_flag) printf("GAME OVER!\n");
    else if(next_arena) printf("YOU WON!\n");

    return 0;
}

void display_tutorial0(){ // tutorial message for the first tutorial arena
    printf("= = = = = = = = = = = =\n|"); 
    printf(" %s  _TEXT_ADVENTURE_  %s", YELLOW, RESET); printf("|\n");
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
}

void display_tutorial1(){ // tutorial message for the second tutorial arena
    printf("= = = = = = = = = = = =\n|"); 
    printf(" %s  _TEXT_ADVENTURE_  %s", YELLOW, RESET); printf("|\n");
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
}

void display_tutorial2(){ // tutorial message for the third tutorial arena
    printf("= = = = = = = = = = = =\n|"); 
    printf(" %s  _TEXT_ADVENTURE_  %s", YELLOW, RESET); printf("|\n");
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
}

void display_tutorial3(){ // tutorial message for the fourth tutorial arena
    printf("= = = = = = = = = = = =\n|"); 
    printf(" %s  _TEXT_ADVENTURE_  %s", YELLOW, RESET); printf("|\n");
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
}

void display_tutorial4(){ // tutorial message for the fifth tutorial arena
    printf("= = = = = = = = = = = =\n|"); 
    printf(" %s  _TEXT_ADVENTURE_  %s", YELLOW, RESET); printf("|\n");
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
}
