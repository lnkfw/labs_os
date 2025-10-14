#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

#define BUFFER_SIZE 1024

int main() {
    int pipe1[2], pipe2[2], pipe3[2];
    
    printf("Parent процесс, PID: %d\n", getpid());
    
    if (pipe(pipe1) == -1 || pipe(pipe2) == -1 || pipe(pipe3) == -1) {
        perror("pipe failed");
        exit(EXIT_FAILURE);
    }
    
    pid_t pid1 = fork();
    if (pid1 == 0) {
        close(pipe1[1]); close(pipe2[0]); close(pipe3[0]); close(pipe3[1]);
        dup2(pipe1[0], STDIN_FILENO);
        dup2(pipe2[1], STDOUT_FILENO);
        execl("./child1", "child1", NULL);
        perror("execl child1 failed");
        exit(EXIT_FAILURE);
    }
    
    pid_t pid2 = fork();
    if (pid2 == 0) {
        close(pipe1[0]); close(pipe1[1]); close(pipe2[1]); close(pipe3[0]);
        dup2(pipe2[0], STDIN_FILENO);
        dup2(pipe3[1], STDOUT_FILENO);
        execl("./child2", "child2", NULL);
        perror("execl child2 failed");
        exit(EXIT_FAILURE);
    }
    
    close(pipe1[0]); close(pipe2[0]); close(pipe2[1]); close(pipe3[1]);
    
    printf("Введите строку:\n");
    
    char input[BUFFER_SIZE], output[BUFFER_SIZE];
    ssize_t bytes_read;
    
    while (fgets(input, BUFFER_SIZE, stdin) != NULL) {
        write(pipe1[1], input, strlen(input));
        bytes_read = read(pipe3[0], output, BUFFER_SIZE - 1);
        if (bytes_read > 0) {
            output[bytes_read] = '\0';
            printf("Итог: %s", output);
        }
    }
    
    close(pipe1[1]); close(pipe3[0]);
    waitpid(pid1, NULL, 0); waitpid(pid2, NULL, 0);
    printf("Parent завершен.\n");
    return 0;
}