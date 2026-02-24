
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>

#define SYNC_FILE    "/tmp/covert_start"
#define BIT_DURATION 1.0  

static double now(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

static void sleep_until(double target)
{
    double remaining = target - now();
    if (remaining <= 0.0) return;
    struct timespec ts;
    ts.tv_sec  = (time_t)remaining;
    ts.tv_nsec = (long)((remaining - ts.tv_sec) * 1e9);
    nanosleep(&ts, NULL);
}

static void hammer_memory(double until)
{
    pid_t pid = fork();
    if (pid == 0) {
        int devnull = open("/dev/null", O_WRONLY);
        if (devnull == -1) {
            perror("open");
            _exit(1);
        }

        dup2(devnull, STDOUT_FILENO);
        dup2(devnull, STDERR_FILENO);

        close(devnull);

        execl("./simple_stream", "simple_stream", NULL);

        perror("execl failed");
        _exit(1);
    }

    else if (pid > 0) {
        while (now() < until) {
            usleep(1000);  
        }
        kill(pid, SIGKILL);
        waitpid(pid, NULL, 0);
    }
    else {
        perror("fork failed");
    }
}

int main(int argc, char *argv[])
{
    const char *bits = NULL;
    for (int i = 1; i < argc; i++)
        if (strcmp(argv[i], "--binary") == 0 && i+1 < argc)
            bits = argv[++i];

    if (!bits) {
        fprintf(stderr, "Usage: %s --binary \"01010101...\"\n", argv[0]);
        return 1;
    }

    double start_time = now() + 2.0;   
    FILE *sf = fopen(SYNC_FILE, "w");
    if (!sf) { perror("fopen sync file"); return 1; }
    fprintf(sf, "%.6f\n", start_time);
    fclose(sf);

    printf("transmitter: sync file written (%s)\n", SYNC_FILE);
    printf("transmitter: bit 0 starts in ~2s\n");
    printf("transmitter: sending %zu bits, %.1fs per bit\n\n", strlen(bits), BIT_DURATION);
    fflush(stdout);

    sleep_until(start_time);

    for (size_t i = 0; i < strlen(bits); i++) {
        char   bit      = bits[i];
        double bit_end  = start_time + (i + 1) * BIT_DURATION;

        printf("transmitter: bit %zu = '%c' -> %s at time = %.3f\n", i, bit, bit == '1' ? "hammering memory" : "sleeping", now());
        fflush(stdout);

        if (bit == '1')
            hammer_memory(bit_end);
        else
            sleep_until(bit_end);
    }

    remove(SYNC_FILE);
    printf("transmitter: done.\n");
    return 0;
}