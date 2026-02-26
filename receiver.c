

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/time.h>


#define SYNC_FILE    "/tmp/covert_start"
#define BIT_DURATION 1.0
#define DEFAULT_BITS 16

double mysecond()
{
        struct timeval tp;
        struct timezone tzp;
        int i;

        i = gettimeofday(&tp,&tzp);
        return ( (double) tp.tv_sec + (double) tp.tv_usec * 1.e-6 );
}

static void sleep_until(double target)
{
    double remaining = target - mysecond();
    if (remaining <= 0.0) return;
    struct timespec ts;
    ts.tv_sec  = (time_t)remaining;
    ts.tv_nsec = (long)((remaining - ts.tv_sec) * 1e9);
    nanosleep(&ts, NULL);
}

static double wait_for_sync(void) //get rid of this and wait for the nxt minute
{
    printf("receiver: waiting for transmitter sync file (%s)...\n", SYNC_FILE);
    fflush(stdout);

    FILE *sf = NULL;
    while (!sf) {
        sf = fopen(SYNC_FILE, "r");
        if (!sf) usleep(50000);   
    }

    double start_time = 0.0;
    fscanf(sf, "%lf", &start_time);
    fclose(sf);

    printf("receiver: sync acquired — bit 0 starts at t=%.3f (now=%.3f)\n\n", start_time, mysecond());
    fflush(stdout);
    return start_time;
}

// static double run_simple_stream(double window_start) //mess around with boundries to get the exact size of the cache. and use that cache to run ntimes measurements
// {
//     sleep_until(window_start+0.001);
//     double begin =mysecond();
//     FILE *fp = popen("./simple_stream", "r");
//     if (!fp) { perror("popen"); return -1.0; }

//     char   line[512];
//     double bw = -1.0;

//     while (fgets(line, sizeof(line), fp)) {
//         if (strncmp(line, "Copy:", 5) == 0) {
//             char *p = line + 5;
//             while (*p == ' ' || *p == '\t') p++;
//             bw = atof(p);
//         }
//     }

//     pclose(fp);
//     printf("took %.3f to run and get bw\n", mysecond()-begin);
//     fflush(stdout);
//     if (bw < 0.0) {
//         fprintf(stderr, "receiver: WARNING - could not parse Copy: line\n");
//         bw = 0.0;
//     }
//     return bw;
// }

static double run_simple_stream(double until)
{
    double total_bw = 0.0;
    int count = 0;

    while (mysecond() < until) {
        FILE *fp = popen("./simple_stream", "r");
        if (!fp) {
            perror("popen");
            return -1.0;
        }

        char line[512];
        double bw = -1.0;

        while (fgets(line, sizeof(line), fp)) {
            if (strncmp(line, "Copy:", 5) == 0) {
                sscanf(line, "%*s %lf", &bw);
            }
        }

        pclose(fp);

        if (bw > 0.0) {
            total_bw += bw;
            count++;
        }
    }

    if (count == 0)
        return 0.0;

    return total_bw / count;
}

int main(int argc, char *argv[])
{
    int    num_bits  = DEFAULT_BITS;
    double threshold = 0.0;

    for (int i = 1; i < argc; i++) {
        if      (strcmp(argv[i], "--bits")      == 0 && i+1 < argc) num_bits  = atoi(argv[++i]);
        else if (strcmp(argv[i], "--threshold") == 0 && i+1 < argc) threshold = atof(argv[++i]);
    }

    printf("receiver: calibrating baseline (transmitter not yet active)...\n");
    fflush(stdout);
    double baseline = run_simple_stream(mysecond());
    if (threshold <= 0.0) {
        threshold = baseline * 0.9;
        printf("receiver: baseline = %.0f MB/s  =>  threshold = %.0f MB/s\n\n",
               baseline, threshold);
    } else {
        printf("receiver: baseline = %.0f MB/s, using fixed threshold = %.0f MB/s\n\n",
               baseline, threshold);
    }
    fflush(stdout);

    double start_time = wait_for_sync();
    
    // sleep_until(start_time - 0.5);
    // run_simple_stream(mysecond());
    char *received = (char *)malloc(num_bits + 1);
    if (!received) { fprintf(stderr, "malloc failed\n"); return 1; }

    for (int i = 0; i < num_bits; i++) { //implement something that checks current time and if it's current time is ahead of where it should be just mark that bit as x
        double window_start = start_time + i * BIT_DURATION;
        double window_end   = window_start + BIT_DURATION;

        // sleep_until(window_start+0.3);
        printf("receiver: [bit %d] window open, running simple_stream at time = %.3f...\n", i, mysecond());
        fflush(stdout);

        double bw  = run_simple_stream(window_start);
        char   bit = (bw < threshold) ? '1' : '0';
        received[i] = bit;


        printf("receiver: bit %2d | Copy rate = %8.0f MB/s | threshold = %.0f | decoded = '%c' \n\n", i, bw, threshold, bit);
        fflush(stdout);

        //sleep_until(window_end);
    }
    received[num_bits] = '\0';

    printf("==========================================\n");
    printf("receiver: received bits -> \"%s\"\n", received);
    printf("==========================================\n");

    free(received);
    return 0;
}