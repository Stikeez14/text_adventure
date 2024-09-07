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

void initialize_arena(char **arena, int rows, int cols) { // initializes the arena by adding elements in the matrix
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            arena[i][j] = '0'; 
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

    arena = create_arena(rows, cols); 
    initialize_arena(arena, rows, cols);
    print_arena(arena, rows, cols);
    free_arena(arena, rows);

    return 0;
}