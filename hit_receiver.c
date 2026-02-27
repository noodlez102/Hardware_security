#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include "cacheutils.h"

#ifndef N
#define N 100000
#endif

#define THRESHOLD 2000   // <-- you MUST calibrate this

size_t repeat_hit(void* addr) {
    size_t time = rdtsc();
    for (int i=0; i<N; i++)
        maccess(addr);
    size_t delta = rdtsc() - time;
    delta = delta / 10;   // histogram scaling
    return delta;
}

int main(int argc, char *argv[]) {

    uint8_t *buf = aligned_alloc(64, 64);
    if (!buf) {
        perror("alloc");
        return 1;
    }

    volatile uint8_t *Y = buf;

    maccess((void*)Y);
    mfence();

    usleep(500000);  

    size_t delta = repeat_hit((void*)Y);

    printf("Measured time: %lu\n", delta);

    if (delta > THRESHOLD) {
        printf("Decoded bit: 1\n");
    } else {
        printf("Decoded bit: 0\n");
    }

    free(buf);
    return 0;
}