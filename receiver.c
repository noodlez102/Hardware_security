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
#define MAX_STREAMS  256

/* ------------------------------------------------------------------ */
/*  Timing helpers                                                      */
/* ------------------------------------------------------------------ */

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

/* ------------------------------------------------------------------ */
/*  Concurrent stream table                                             */
/* ------------------------------------------------------------------ */

typedef struct {
    pid_t pid;
    int   read_fd;
    int   index;      /* which bit window this belongs to */
} StreamEntry;

static StreamEntry stream_table[MAX_STREAMS];
static int         stream_count = 0;

/* Fork ./simple_stream, capture its stdout via a pipe, store in table. */
static void stream_launch(int index)
{
    if (stream_count >= MAX_STREAMS) {
        fprintf(stderr, "receiver: too many streams\n");
        return;
    }

    int pipefd[2];
    if (pipe(pipefd) == -1) { perror("pipe"); return; }

    pid_t pid = fork();
    if (pid == 0) {
        /* child */
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        dup2(pipefd[1], STDERR_FILENO);
        close(pipefd[1]);
        execl("./simple_stream", "simple_stream", NULL);
        perror("execl");
        _exit(1);
    } else if (pid > 0) {
        /* parent */
        close(pipefd[1]);
        stream_table[stream_count++] = (StreamEntry){
            .pid     = pid,
            .read_fd = pipefd[0],
            .index   = index
        };
    } else {
        perror("fork");
        close(pipefd[0]);
        close(pipefd[1]);
    }
}

/* Parse a "Copy:  XXXXX" line out of a buffer, return MB/s or -1. */
static double parse_copy_bw(char *buf, int len)
{
    buf[len] = '\0';
    double bw = -1.0;
    char *line = strtok(buf, "\n");
    while (line) {
        if (strncmp(line, "Copy:", 5) == 0) {
            char *p = line + 5;
            while (*p == ' ' || *p == '\t') p++;
            bw = atof(p);
        }
        line = strtok(NULL, "\n");
    }
    return bw;
}

/*
 * Drain all launched streams, decode each bit, fill received[].
 * received must be at least stream_count chars + NUL.
 */
static void stream_gather(char *received, double threshold)
{
    for (int i = 0; i < stream_count; i++) {
        StreamEntry *e = &stream_table[i];

        /* Drain pipe — blocks until child closes its write end (i.e. exits). */
        char    accum[65536];
        int     accum_len = 0;
        char    tmp[4096];
        ssize_t n;

        /* First reap the child (blocking), then drain. */
        waitpid(e->pid, NULL, 0);
        while ((n = read(e->read_fd, tmp, sizeof(tmp) - 1)) > 0) {
            if (accum_len + n < (int)sizeof(accum) - 1) {
                memcpy(accum + accum_len, tmp, n);
                accum_len += n;
            }
        }
        close(e->read_fd);

        double bw  = parse_copy_bw(accum, accum_len);
        if (bw < 0.0) {
            fprintf(stderr, "receiver: WARNING - could not parse Copy: line for bit %d\n", e->index);
            bw = 0.0;
        }

        char bit = (bw < threshold) ? '1' : '0';
        received[e->index] = bit;

        printf("receiver: bit %2d | Copy rate = %8.0f MB/s | threshold = %.0f | decoded = '%c'\n",
               e->index, bw, threshold, bit);
        fflush(stdout);
    }

    stream_count = 0;   /* reset for potential reuse */
}

/* ------------------------------------------------------------------ */
/*  Calibration (single blocking run, same as before)                  */
/* ------------------------------------------------------------------ */

static double run_simple_stream_once(void)
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

    char    accum[65536];
    int     accum_len = 0;
    char    tmp[4096];
    ssize_t n;

    waitpid(pid, NULL, 0);
    while ((n = read(pipefd[0], tmp, sizeof(tmp) - 1)) > 0) {
        if (accum_len + n < (int)sizeof(accum) - 1) {
            memcpy(accum + accum_len, tmp, n);
            accum_len += n;
        }
    }
    close(pipefd[0]);

    double bw = parse_copy_bw(accum, accum_len);
    if (bw < 0.0) {
        fprintf(stderr, "receiver: WARNING - could not parse Copy: line during calibration\n");
        bw = 0.0;
    }
    return bw;
}

/* ------------------------------------------------------------------ */
/*  Sync                                                                */
/* ------------------------------------------------------------------ */

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

/* ------------------------------------------------------------------ */
/*  Main                                                                */
/* ------------------------------------------------------------------ */

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
    double baseline = run_simple_stream_once();
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

    char *received = (char *)malloc(num_bits + 1);
    if (!received) { fprintf(stderr, "malloc failed\n"); return 1; }
    memset(received, '?', num_bits);
    received[num_bits] = '\0';

    /* ---- Launch all streams, one per bit window, at the right time ---- */
    for (int i = 0; i < num_bits; i++) {
        double window_start = start_time + i * BIT_DURATION;
        sleep_until(window_start);

        printf("receiver: [bit %d] launching simple_stream at t=%.3f\n", i, now());
        fflush(stdout);

        stream_launch(i);

        /* simple_stream finishes well within one BIT_DURATION second,
           so the next sleep_until will naturally pace us — no extra sleep needed. */
    }

    /* ---- Gather all results after every bit window has been launched ---- */
    printf("\nreceiver: all streams launched, gathering results...\n\n");
    fflush(stdout);

    stream_gather(received, threshold);

    printf("\n==========================================\n");
    printf("receiver: received bits -> \"%s\"\n", received);
    printf("==========================================\n");

    free(received);
    return 0;
}