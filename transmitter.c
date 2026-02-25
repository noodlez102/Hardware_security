
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/time.h>

#define SYNC_FILE    "/tmp/covert_start"
#define BIT_DURATION 10.0  

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

static void hammer_memory(double until) { //double check to make sure it kills properly
    while (mysecond() < until) {
        pid_t pid = fork();
        if (pid == 0) {
            int devnull = open("/dev/null", O_WRONLY);
            dup2(devnull, STDOUT_FILENO);
            dup2(devnull, STDERR_FILENO);
            close(devnull);
            execl("./simple_stream", "simple_stream", NULL);
            perror("execl failed");
            _exit(1);
        } else if (pid > 0 ) {
            while(1){
                if (until > 0.0 && mysecond() >= until) {
                    kill(pid, SIGKILL);
                    waitpid(pid, NULL, 0);
                    break;
                }
            }

        }
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

    double start_time = mysecond() + 2.0;   //get rid of this and sync up at the nxt minuite or something
    FILE *sf = fopen(SYNC_FILE, "w");
    if (!sf) { perror("fopen sync file"); return 1; }
    fprintf(sf, "%.6f\n", start_time);
    fclose(sf);

    printf("transmitter: bit 0 starts in ~2s\n");

    for (size_t i = 0; i < strlen(bits); i++) {
        char bit = bits[i];
        double bit_start = start_time + i * BIT_DURATION;
        double bit_end = start_time + (i + 1) * BIT_DURATION;
        
        if(i==0){
            sleep_until(bit_start-1); 
        }else{
            sleep_until(bit_start); 
        }

        printf("transmitter: bit %zu = '%c' -> %s starting at time = %.3f\n", i, bit, bit == '1' ? "hammered" : "slept", mysecond());
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