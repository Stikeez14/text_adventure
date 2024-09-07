#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>

#define MAX_LINE_LENGTH 1024

/* only for linux */
void enable_raw_mode() { 
    struct termios term;
    tcgetattr(STDIN_FILENO, &term);
    term.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &term);
}

/* only for linux */
void disable_raw_mode() { 
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
                    arena[i][j] = ' ';  // Fill with spaces if line is shorter than expected
                }
            }
        } else {
            // Fill the rest of the arena with spaces if file has fewer lines
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
            printf("%c ", arena[i][j]);
        }
        printf("\n");
    }
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

    while (1) { // game loop

        clear_console();
        print_arena(arena, rows, cols);  
    
        if(!death_flag) input = getchar(); 
        else break;

        arena[player_x][player_y] = ' '; // clear current player location

        if ((input == 'w' || input == 'W') && player_x > 1 && arena[player_x - 1][player_y] != '=' && arena[player_x - 1][player_y] != '|') player_x--;         
        else if ((input == 's' || input == 'S') && player_x < rows - 2 && arena[player_x + 1][player_y] != '=' && arena[player_x + 1][player_y] != '|') player_x++;  
        else if ((input == 'a' || input == 'A') && player_y > 1 && arena[player_x][player_y - 1] != '=' && arena[player_x][player_y - 1] != '|') player_y--;     
        else if ((input == 'd' || input == 'D') && player_y < cols - 2 && arena[player_x][player_y + 1] != '=' && arena[player_x][player_y + 1] != '|') player_y++;  
        else if (input == 'x') break;   

        if ((input == 'w' || input == 'W' || input == 's' || input == 'S' || input == 'a' || input == 'A' || input == 'd' || input == 'D') && arena[player_x][player_y] == 'x') death_flag = 1;
        
        arena[player_x][player_y] = 'p'; // update player location on the arena

    }

    disable_raw_mode();

    free_arena(arena, rows);

    if(death_flag) printf("GAME OVER!\n");

    return 0;
}
