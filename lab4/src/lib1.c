#include "calc.h"

int GCF(int A, int B) { // алг евклида
    while (B != 0) {
        int tmp = A % B;
        A = B;
        B = tmp;
    }
    return A;
}

float Square(float A, float B) { // прямоугольник
    return A * B;
}
