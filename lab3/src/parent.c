#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/mman.h>
#include <string.h>
#include <fcntl.h>
#include <sys/wait.h>

static char* to_child1 = NULL;
static char* from_child2 = NULL;
static const size_t SH = 4096;
static volatile sig_atomic_t ready_flag = 0;
static volatile sig_atomic_t stage = 0;
static pid_t c1 = 0, c2 = 0;

void sig_handler(int s) {
    if (s == SIGUSR2) {
        if (stage == 1) {
            stage = 2;
            kill(c2, SIGUSR1);
        } else if (stage == 2) {
            ready_flag = 1;
            stage = 0;
        }
    }
}

int main() {
    int f1 = open("shared1.dat", O_RDWR | O_CREAT | O_TRUNC, 0666);
    int f2 = open("shared2.dat", O_RDWR | O_CREAT | O_TRUNC, 0666);
    int f3 = open("shared3.dat", O_RDWR | O_CREAT | O_TRUNC, 0666);

    if (f1 < 0 || f2 < 0 || f3 < 0) { perror("open"); return 1; }

    if (ftruncate(f1, SH) < 0 || ftruncate(f2, SH) < 0 || ftruncate(f3, SH) < 0) {
        perror("ftruncate"); return 1;
    }
    to_child1 = mmap(NULL, SH, PROT_WRITE | PROT_READ, MAP_SHARED, f1, 0);
    from_child2 = mmap(NULL, SH, PROT_WRITE | PROT_READ, MAP_SHARED, f3, 0);
    close(f1); close(f2); close(f3);

    c1 = fork();
    if (c1 == 0) execl("./child1", "./child1", NULL);
    if (c1 < 0) { perror("ошибка форка c1"); return 1; }

    c2 = fork();
    if (c2 == 0) execl("./child2", "./child2", NULL);
    if (c2 < 0) { perror("ошибка форка c2"); kill(c1, SIGTERM); return 1; }

    signal(SIGUSR2, sig_handler);

    while (1) {
        char input[SH];
        fflush(stdout);

        if (!fgets(input, SH, stdin)){ 
            break;
        }
        if (strcmp(input, "exit\n") == 0){ 
            break;
        }
        memset(to_child1, 0, SH);
        memcpy(to_child1, input, strlen(input) + 1);
        stage = 1;
        ready_flag = 0;
        kill(c1, SIGUSR1);
        while (!ready_flag) pause();

        printf("%s\n", from_child2);
    }
    kill(c1, SIGTERM);
    kill(c2, SIGTERM);

    wait(NULL);
    wait(NULL);

    if (to_child1 && to_child1 != MAP_FAILED) munmap(to_child1, SH);
    if (from_child2 && from_child2 != MAP_FAILED) munmap(from_child2, SH);
    return 0;
}
