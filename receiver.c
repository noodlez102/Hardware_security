

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>

#define SYNC_FILE    "/tmp/covert_start"
#define BIT_DURATION 1.0
#define DEFAULT_BITS 16

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

static double wait_for_sync(void)
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

    printf("receiver: sync acquired — bit 0 starts at t=%.3f (now=%.3f)\n\n",
           start_time, now());
    fflush(stdout);
    return start_time;
}

// static double run_simple_stream(void)
// {
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

//     if (bw < 0.0) {
//         fprintf(stderr, "receiver: WARNING - could not parse Copy: line\n");
//         bw = 0.0;
//     }
//     return bw;
// }

// static double run_simple_stream(double until)
// {
    // int pipefd[2];
    // if (pipe(pipefd) == -1) {
    //     perror("pipe");
    //     return -1.0;
    // }

    // pid_t pid = fork();

    // if (pid == 0) {
    //     // ---- CHILD ----
    //     close(pipefd[0]); // close read end

    //     dup2(pipefd[1], STDOUT_FILENO);
    //     dup2(pipefd[1], STDERR_FILENO);
    //     close(pipefd[1]);

    //     execl("./simple_stream", "simple_stream", NULL);
    //     perror("execl");
    //     _exit(1);
    // }

    // // ---- PARENT ----
    // close(pipefd[1]);  // close write end

    // // Make pipe non-blocking
    // fcntl(pipefd[0], F_SETFL, O_NONBLOCK);

    // char buffer[512];
    // double bw = -1.0;

    // while (now() < until) {

    //     ssize_t n = read(pipefd[0], buffer, sizeof(buffer) - 1);
    //     if (n > 0) {
    //         buffer[n] = '\0';

    //         char *line = strtok(buffer, "\n");
    //         while (line) {
    //             if (strncmp(line, "Copy:", 5) == 0) {
    //                 char *p = line + 5;
    //                 while (*p == ' ' || *p == '\t') p++;
    //                 bw = atof(p);
    //             }
    //             line = strtok(NULL, "\n");
    //         }
    //     }

    //     usleep(1000);  // small sleep to reduce spin
    // }

    // // Kill STREAM at end of bit window
    // kill(pid, SIGKILL);
    // waitpid(pid, NULL, 0);
    // close(pipefd[0]);

    // return bw;
// }

static double run_simple_stream()
{
    int pipefd[2];
    if (pipe(pipefd) == -1) { perror("pipe"); return -1.0; }

    pid_t pid = fork();
    if (pid == 0) {
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        dup2(pipefd[1], STDERR_FILENO);
        close(pipefd[1]);
        execl("./simple_stream", "simple_stream", NULL);
        perror("execl");
        _exit(1);
    }

    close(pipefd[1]);
    fcntl(pipefd[0], F_SETFL, O_NONBLOCK);

    char   accum[65536];
    int    accum_len = 0;
    double bw        = -1.0;

    while (1) {
        int   status;
        pid_t ret = waitpid(pid, &status, WNOHANG);
        if (ret == pid) {
            char    tmp[4096];
            ssize_t n;
            while ((n = read(pipefd[0], tmp, sizeof(tmp) - 1)) > 0)
                if (accum_len + n < (int)sizeof(accum) - 1) {
                    memcpy(accum + accum_len, tmp, n);
                    accum_len += n;
                }
            break;
        }

        // if (until > 0.0 && now() >= until) {
        //     kill(pid, SIGKILL);
        //     waitpid(pid, NULL, 0);
        //     break;
        // }

        char    tmp[4096];
        ssize_t n = read(pipefd[0], tmp, sizeof(tmp) - 1);
        if (n > 0 && accum_len + n < (int)sizeof(accum) - 1) {
            memcpy(accum + accum_len, tmp, n);
            accum_len += n;
        }

        usleep(5000);  /* poll every 5ms */
    }

    close(pipefd[0]);

    /* Parse accumulated output for the Copy: line */
    accum[accum_len] = '\0';
    char *line = strtok(accum, "\n");
    while (line) {
        if (strncmp(line, "Copy:", 5) == 0) {
            char *p = line + 5;
            while (*p == ' ' || *p == '\t') p++;
            bw = atof(p);
        }
        line = strtok(NULL, "\n");
    }

    if (bw < 0.0) {
        fprintf(stderr, "receiver: WARNING - could not parse Copy: line\n");
        bw = 0.0;
    }
    return bw;
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
    double baseline = run_simple_stream();
    if (threshold <= 0.0) {
        threshold = baseline * 0.94;
        printf("receiver: baseline = %.0f MB/s  =>  threshold = %.0f MB/s\n\n",
               baseline, threshold);
    } else {
        printf("receiver: baseline = %.0f MB/s, using fixed threshold = %.0f MB/s\n\n",
               baseline, threshold);
    }
    fflush(stdout);

    double start_time = wait_for_sync();
    
    sleep_until(start_time - 0.5);
    run_simple_stream();
    char *received = (char *)malloc(num_bits + 1);
    if (!received) { fprintf(stderr, "malloc failed\n"); return 1; }

    for (int i = 0; i < num_bits; i++) {
        double window_start = start_time + i * BIT_DURATION;
        double window_end   = window_start + BIT_DURATION;

        sleep_until(window_start+0.3);
        printf("receiver: [bit %d] window open, running simple_stream at time = %.3f...\n", i, now());
        fflush(stdout);

        double bw  = run_simple_stream();
        char   bit = (bw < threshold) ? '1' : '0';
        received[i] = bit;


        printf("receiver: bit %2d | Copy rate = %8.0f MB/s | threshold = %.0f | decoded = '%c' \n\n",
               i, bw, threshold, bit);
        fflush(stdout);

        sleep_until(window_end);
    }
    received[num_bits] = '\0';

    printf("==========================================\n");
    printf("receiver: received bits -> \"%s\"\n", received);
    printf("==========================================\n");

    free(received);
    return 0;
}