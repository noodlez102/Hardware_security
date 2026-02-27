#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <x86intrin.h>
#include <unistd.h>
#include "cacheutils.h"
#include <sys/time.h>
#include <time.h>
#include <fcntl.h>
#include <signal.h>
#include <math.h>

#ifndef N
#define N 1
#endif

#define THRESHOLD 2000 

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

    usleep(500000);  

    double avg =0;
    for(int i=0; i< 1024*16; i++){
        size_t delta = repeat_hit((void*)Y);
        avg+=delta;
    }

    avg=avg/(1024*16);
    printf("Measured time: %lu\n", avg);

    if (avg > THRESHOLD) {
        printf("Decoded bit: 1\n");
    } else {
        printf("Decoded bit: 0\n");
    }

    free(buf);
    return 0;
}