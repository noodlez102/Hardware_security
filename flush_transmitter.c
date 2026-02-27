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

#define WAYS 12
#define C (2 * 1024 * 1024)   // 2 MiB
#define DURATION 5.0

double mysecond()
{
        struct timeval tp;
        struct timezone tzp;
        int i;

        i = gettimeofday(&tp,&tzp);
        return ( (double) tp.tv_sec + (double) tp.tv_usec * 1.e-6 );
}

int main(int argc, char *argv[]) {

    int bit = -1;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--binary") == 0 && i + 1 < argc)
            bit = atoi(argv[++i]);
    }

    if (bit != 0 && bit != 1) {
        printf("Usage: %s --binary 0|1\n", argv[0]);
        return 1;
    }

    size_t bufsize = WAYS * C + 64;
    uint8_t *buf = aligned_alloc(64, bufsize);

    if (!buf) {
        perror("alloc");
        return 1;
    }

    volatile uint8_t *evset[WAYS];

    for (int i = 0; i < WAYS; i++)
        evset[i] = buf + i * C;

    printf("Transmitting bit %d...\n", bit);
    double end =mysecond() + DURATION;
    while (mysecond() < end) {
        if (bit == 1) {
            for (int i = 0; i < WAYS; i++)
                flush((void*)evset[i]);
                usleep(500000);  
        }else{
            usleep(500000);  
        }
    }

    free(buf);
    return 0;
}