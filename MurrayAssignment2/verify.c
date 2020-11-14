#include <stdio.h>

int main(void) {
    unsigned int a[36] = {0,0,0,0,0,0,0,0,9,4,7,9,0,12,14,15,16,11,0,2,3,4,5,6,0,4,3,2,1,2,0,2,7,6,4,9};
    unsigned int b[36] = {0,0,0,0,0,0,0,0,9,4,7,9,0,12,14,15,16,11,0,2,3,4,5,6,0,4,3,2,1,2,0,2,7,6,4,9};

    // Modified a and b
    // unsigned int a[36] = {1,3,5,0,0,0,0,0,9,4,7,9,0,12,14,15,16,11,0,2,3,4,5,6,0,4,3,2,1,2,0,2,7,6,4,9};
    // unsigned int b[36] = {2,4,6,0,0,0,0,0,9,4,7,9,0,12,14,15,16,11,0,2,3,4,5,6,0,4,3,2,1,2,0,2,7,6,4,9};

    for (int i = 0; i < 6; i++) {
        for (int j = 0; j < 6; j++) {
            unsigned int c = 0;
            for (int k = 0; k < 6; k++) {
                c += a[i * 6 + k] * b[k * 6 + j];
            }
            printf("%d ", c);
        }
        printf("\n");
    }
}