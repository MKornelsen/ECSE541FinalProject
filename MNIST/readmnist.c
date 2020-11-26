#include <stdio.h>

int main(void) {
    FILE *fp = fopen("train-images.idx3-ubyte", "rb");
    if (fp == NULL) {
        printf("fopen failed\n");
    }

    unsigned char buffer[16];
    fread(buffer, 1, sizeof(buffer), fp);
    for (int i = 0; i < sizeof(buffer); i++) {
        printf("%02X ", buffer[i]);
    }
    printf("\n");
    unsigned char flip[4];
    for (int i = 0; i < sizeof(buffer) / 4; i++) {
        for (int j = 0; j < 4; j++) {
            flip[j] = buffer[i * 4 + 3 - j];
        }
        printf("%d ", *((unsigned int *) flip));
    }

    fclose(fp);

    return 0;
}