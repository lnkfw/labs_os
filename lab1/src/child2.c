#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>

#define BUFFER_SIZE 1024

int main() {
    printf("Child2 запущен, PID: %d\n", getpid());
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    
    while ((bytes_read = read(STDIN_FILENO, buffer, BUFFER_SIZE)) > 0) {
        for (int i = 0; i < bytes_read; i++) 
            if (isspace(buffer[i])) buffer[i] = '_';
        write(STDOUT_FILENO, buffer, bytes_read);
    }
    
    printf("Child2 завершен\n");
    return 0;
}