#include <stdio.h>
#include "calc.h"

int main() {
    int cmd;

    while (1) {
        if (scanf("%d", &cmd) != 1)
            break;

        if (cmd == 1) {
            int a, b;
            scanf("%d %d", &a, &b);
            printf("НОД = %d\n", GCF(a, b));
        } 
        else if (cmd == 2) {
            float a, b;
            scanf("%f %f", &a, &b);
            printf("площадь = %.2f\n", Square(a, b));
        }
    }

    return 0;
}
