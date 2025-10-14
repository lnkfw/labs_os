#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>

#define BUFFER_SIZE 1024

int main() {
    printf("Child1 запущен, PID: %d\n", getpid());
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    
    while ((bytes_read = read(STDIN_FILENO, buffer, BUFFER_SIZE)) > 0) {
        for (int i = 0; i < bytes_read; i++) buffer[i] = tolower(buffer[i]);
        write(STDOUT_FILENO, buffer, bytes_read);
    }
    
    printf("Child1 завершен\n");
    return 0;
}