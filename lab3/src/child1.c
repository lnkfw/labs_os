#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>

static char* indata = NULL;
static char* outdata = NULL;
static const size_t SH = 4096;
static volatile sig_atomic_t stop_flag = 0;

void sig_handler(int s) {
    if (s == SIGTERM) {
        stop_flag = 1;
        return;
    }
    if (s == SIGUSR1) {
        char buf[SH];
        memcpy(buf, indata, SH);
        size_t len = strnlen(buf, SH);
        for (size_t i = 0; i < len; i++) {
            if (buf[i] >= 'A' && buf[i] <= 'Z')
                buf[i] = buf[i] + ('a' - 'A');
        }
        memset(outdata, 0, SH);
        memcpy(outdata, buf, len + 1);
        kill(getppid(), SIGUSR2);
    }
}

int main() {
    int f1 = open("shared1.dat", O_RDONLY);
    if (f1 < 0) { perror("ошибка shared1.dat"); return 1; }
    int f2 = open("shared2.dat", O_RDWR);
    if (f2 < 0) { perror("ошибка shared2.dat"); close(f1); return 1; }

    indata = mmap(NULL, SH, PROT_READ, MAP_SHARED, f1, 0);
    outdata = mmap(NULL, SH, PROT_WRITE | PROT_READ, MAP_SHARED, f2, 0);

    close(f1);
    close(f2);

    signal(SIGUSR1, sig_handler);
    signal(SIGTERM, sig_handler);

    while (!stop_flag) pause();

    if (indata && indata != MAP_FAILED) munmap(indata, SH);
    if (outdata && outdata != MAP_FAILED) munmap(outdata, SH);
    return 0;
}
