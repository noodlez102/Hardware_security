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

#define WAYS 12
#define C (2 * 1024 * 1024)   // 2 MiB
#define THRESHOLD 29

size_t repeat_hit(void* addr) {
    size_t time = rdtsc();
    for (int i=0; i<N; i++)
        maccess(addr);
    size_t delta = rdtsc() - time;
    delta = delta / 10;   // histogram scaling
    return delta;
}

int main(int argc, char *argv[]) {

    size_t bufsize = WAYS * C + 64;
    uint8_t *buf = aligned_alloc(64, bufsize);

    if (!buf) {
        perror("alloc");
        return 1;
    }

    volatile uint8_t *evset[WAYS];

    for (int i = 0; i < WAYS; i++)
        evset[i] = buf + i * C;

    double avg =0;
    for(int i=0; i< 1024*16; i++){
        for (int j = 0; j < WAYS; i++){
            printf("at iteration %f %f\n",i,j)''
            size_t delta = repeat_hit((void*)evset[j]);
            avg+=delta;
        }
    }

    avg=avg/(1024*16);
    printf("Measured time: %f\n", avg);

    if (avg > THRESHOLD) {
        printf("Decoded bit: 1\n");
    } else {
        printf("Decoded bit: 0\n");
    }

    free(buf);
    return 0;
}