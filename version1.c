#include <stdio.h>
#include <stdlib.h>

char** create_arena(int rows, int cols) { // allocates memory for the arena
    char **arena = (char **)malloc(rows * sizeof(char *));
    for (int i = 0; i < rows; i++){
        arena[i] = (char *)malloc(cols * sizeof(char));
    }
    return arena;
}

void free_arena(char **arena, int rows) { // frees the allocated memory
    for (int i = 0; i < rows; i++) {
        free(arena[i]);
    }
    free(arena);
}

void initialize_arena(char **arena, int rows, int cols) { // initializes the arena borders
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            if (i == 0) arena[i][j] = '='; 
            else if (i > 0 && i < (cols - 1) && j == 0) arena[i][j] = '|';
            else if (i > 0 && i < (cols - 1 ) && j == (cols - 1)) arena[i][j] = '|';
            else if (i == rows - 1) arena[i][j] = '=';
            else arena[i][j] = ' ';
        }
    }
}

void print_arena(char **arena, int rows, int cols) { // prints all the matrix elements
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            printf("%c ", arena[i][j]);
        }
        printf("\n");
    }
}

int main(){

    int rows = 10, cols = 10;
    char **arena;  // dynamically allocated 2D array 
    
    int x = 5, y = 5; // player coordinates   
     
    char input; // keyboard input

    arena = create_arena(rows, cols);
    initialize_arena(arena, rows, cols);

    arena[5][5] = 'p'; // initialize player in the arena

    while (1) { // game loop
    
        system("clear"); 

        print_arena(arena, rows, cols);   
    
        input = getchar();  // get keyboard input

        arena[x][y] = ' '; // clear current player location

        if ((input == 'w' || input == 'W') && x > 1) x--;         
        else if ((input == 's' || input == 'S') && x < rows - 2) x++;  
        else if ((input == 'a' || input == 'A') && y > 1) y--;     
        else if ((input == 'd' || input == 'D') && y < cols - 2) y++;  
        else if (input == 'x') break;            

        arena[x][y] = 'p'; // update player location on the arena
    }

    free_arena(arena, rows);

    return 0;
}