#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>

#define MAX_LINE_LENGTH 1024

#define RESET "\033[0m"
#define RED "\033[31m"
#define CYAN "\033[36m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"

#define MAX_INVENTORY_ITEMS 3

 
/*
    GLOBAL VARIABLES
*/
int player_h = 100;

char items[MAX_INVENTORY_ITEMS] = {'\0','\0','\0'};

int weapon_flag = 0;

/* 
    RAW MODE FOR INPUT & CONSOLE CLEAR
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
            if (arena[i][j] == 'w' || arena[i][j] == 'x') printf("%s%c %s", RED, arena[i][j], RESET); // enemies ~ RED
            else if (arena[i][j] == '#') printf("%s%c %s", YELLOW, arena[i][j], RESET); // exit ~ YELLOW
            else if (arena[i][j] == '+' || arena[i][j] == '^') printf("%s%c %s", GREEN, arena[i][j], RESET); // consumables ~ GREEN
            else if (arena[i][j] == 'p') printf("%s%c %s", CYAN, arena[i][j], RESET); // player ~ CYAN
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
        printf("|"); printf("%sH%s", RED, RESET); printf(":");
        printf("%s0%d%s", RED, health, RESET); printf("|");
    }
    else if (health < 10 && health >= 1) { // print player health with red for [1,10)
        printf("|"); printf("%sH%s", RED, RESET); printf(":");
        printf("%s00%d%s", RED, health, RESET); printf("|");
    }
    else printf("|H:000|");
}

void handle_consumables(char items[]){

    for(int i = 0; i < MAX_INVENTORY_ITEMS; i++) {
        if (items[i] != '\0'){ 
            printf("%s[%s", CYAN, RESET); printf("%s%c%s", GREEN, items[i], RESET); printf("%s]%s", CYAN, RESET);
        }  
        else printf("%s[ ]%s", CYAN, RESET);  
       
        if (i < MAX_INVENTORY_ITEMS) {
            printf("|");  
        }
    }
    printf("\n= = = = = = = = = =\n");
}

int handle_input(int *player_x, int *player_y, char **arena, int rows, int cols) {
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
            if (*player_x > 1 && arena[*player_x - 1][*player_y] != '=' && arena[*player_x - 1][*player_y] != '|') 
                (*player_x)--;
            break;
        case 's': case 'S': // move down
            if (*player_x < rows - 2 && arena[*player_x + 1][*player_y] != '=' && arena[*player_x + 1][*player_y] != '|') 
                (*player_x)++;
            break;
        case 'a': case 'A': // move left
            if (*player_y > 1 && arena[*player_x][*player_y - 1] != '=' && arena[*player_x][*player_y - 1] != '|') 
                (*player_y)--;
            break;
        case 'd': case 'D': // move right
            if (*player_y < cols - 2 && arena[*player_x][*player_y + 1] != '=' && arena[*player_x][*player_y + 1] != '|') 
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
            return 0; // Inventory is not full
        }
    }
    return 1; // Inventory is full
}

int main() {

    int rows, cols;
    const char *arena_file = "arena1.txt";

    get_arena_dimensions(arena_file, &rows, &cols); // get the number of rows and cols from arena file

    char **arena = create_arena(rows, cols); // create the arena based on the rows and cols

    initialize_arena(arena, rows, cols, arena_file); 

    int player_x, player_y; // coordinates for player

    for (int i = 0; i < rows; i++) { // find in the file the player position and update x & y
        for (int j = 0; j < cols; j++) {
            if (arena[i][j] == 'p') {
                player_x = i;
                player_y = j;
                break;
            }
        }
    }

    enable_raw_mode();

    char input; // stores keyboard input
    
    int death_flag = 0;
    int spike_flag = 0; 
    int exit_game = 0;
    int warrior_flag = 0;
    int next_arena = 0; 

    int over_health_consumable = 0; // flags for knowing when the player is on top of items
    int over_attack_consumable = 0;

    while (!exit_game) { // game loop

        clear_console();
        printf("= = = = = = = = = =\n| TEXT_ADVENTURE_ |\n");
        print_arena(arena, rows, cols);

        handle_player_health(player_h);
        handle_consumables(items);
    
        if(player_h <= 0) { // if health reaches 0 ~ death flag and break the loop
            death_flag = 1;
            break; 
        }
        else if(next_arena) break; // if '#' is reached ~ arena is leaved ~ break the loop

        if(spike_flag){ // if the player is over a spike 'x' ~ after leaving that position the spike remains there
            arena[player_x][player_y] = 'x'; 
            spike_flag = 0;
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

        exit_game = handle_input(&player_x, &player_y, arena, rows, cols);   

        if (arena[player_x][player_y] == 'x'){ // -30 health if the player is on top of a spike
            player_h -= 30; 
            spike_flag = 1; 
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

        if (arena[player_x][player_y] == '#') next_arena = 1; 

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
        
        arena[player_x][player_y] = 'p'; // update player location on the arena

        if(warrior_flag) arena[player_x][player_y] = 'w'; // if 'w' kills 'p' he overrides the player on the map
        else if (next_arena) arena[player_x][player_y] = '#'; // if 'p' reaches the exit, '#' overrides the player
        else if (spike_flag && player_h <= 0) arena[player_x][player_y] = 'x'; // 'p' overrides a spike when he s on
                                                                               // as long as he s not dead
    }

    disable_raw_mode();

    free_arena(arena, rows);

    if(death_flag) printf("GAME OVER!\n");
    else if(next_arena) printf("YOU WON!\n");

    return 0;
}
