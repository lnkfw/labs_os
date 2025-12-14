#include "calc.h"

int GCF(int A, int B) { // наивный алг
    int min = A < B ? A : B;
    for (int i = min; i >= 1; --i) {
        if (A % i == 0 && B % i == 0) {
            return i;
        }
    }
    return 1;
}

float Square(float A, float B) { // прямоугольный треугольник
    return (A * B) / 2.0f;
}
