#include <stdio.h>
#include <dlfcn.h>
#include "calc.h"

typedef int (*gcf_func)(int, int);
typedef float (*square_func)(float, float);

int main() {
    void* handle = NULL;
    gcf_func GCF_dyn = NULL;
    square_func Square_dyn = NULL;

    int current = 1;
    handle = dlopen("./libcalc1.so", RTLD_LAZY);

    GCF_dyn = (gcf_func)dlsym(handle, "GCF");
    Square_dyn = (square_func)dlsym(handle, "Square");

    int cmd;

    while (1) {
        if (scanf("%d", &cmd) != 1)
            break;

        if (cmd == 0) {
            dlclose(handle);

            if (current == 1) {
                handle = dlopen("./libcalc2.so", RTLD_LAZY);
                current = 2;
                printf("переключено на 2ую реализацию\n");
            } else {
                handle = dlopen("./libcalc1.so", RTLD_LAZY);
                current = 1;
                printf("переключено на 1ую реализацию\n");
            }

            GCF_dyn = (gcf_func)dlsym(handle, "GCF");
            Square_dyn = (square_func)dlsym(handle, "Square");
        }
        else if (cmd == 1) {
            int a, b;
            scanf("%d %d", &a, &b);
            printf("НОД = %d\n", GCF_dyn(a, b));
        }
        else if (cmd == 2) {
            float a, b;
            scanf("%f %f", &a, &b);
            printf("площадь = %.2f\n", Square_dyn(a, b));
        }
    }

    dlclose(handle);
    return 0;
}
