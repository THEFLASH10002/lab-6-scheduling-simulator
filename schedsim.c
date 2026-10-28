#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "process.h"
#include "util.h"

/** Add each newly eligible process to the circular FIFO queue exactly once.
 *  Arrival time is the primary ordering key; source order breaks arrival ties.
 */
static void enqueueArrivals(ProcessType plist[], const int remaining[], int admitted[],
                            int n, int now, int ready[], int *tail, int *count)
{
    for (;;) {
        int next = -1;
        for (int i = 0; i < n; ++i) {
            if (admitted[i] || remaining[i] <= 0 || plist[i].art > now)
                continue;
            if (next < 0 || plist[i].art < plist[next].art) next = i;
        }
        if (next < 0) return;
        admitted[next] = 1;
        ready[*tail] = next;
        *tail = (*tail + 1) % n;
        ++*count;
    }
}

/** Calculate RR waiting times using a fixed time quantum and FIFO ready queue.
 *  A process arriving during a time slice precedes the preempted process.
 *  When the queue is empty, advance directly to the next arrival.
 */
void findWaitingTimeRR(ProcessType plist[], int n, int quantum)
{
    if (n <= 0 || quantum <= 0) return;
    int *remaining = malloc((size_t)n * sizeof(int));
    int *admitted = calloc((size_t)n, sizeof(int));
    int *ready = malloc((size_t)n * sizeof(int));
    if (!remaining || !admitted || !ready) exit(EXIT_FAILURE);
    int completed = 0, time = 0, head = 0, tail = 0, count = 0;
    for (int i = 0; i < n; ++i) {
        remaining[i] = plist[i].bt;
        plist[i].wt = 0;
        if (remaining[i] <= 0) completed++;
    }
    while (completed < n) {
        enqueueArrivals(plist, remaining, admitted, n, time, ready, &tail, &count);
        if (count == 0) {
            /* An idle CPU must not accrue any waiting time. */
            int next = INT_MAX;
            for (int i = 0; i < n; ++i) {
                if (remaining[i] > 0 && !admitted[i] && plist[i].art < next)
                    next = plist[i].art;
            }
            if (next == INT_MAX) break;
            time = next;
            continue;
        }
        int current = ready[head];
        head = (head + 1) % n;
        count--;
        int slice = remaining[current] < quantum ? remaining[current] : quantum;
        time += slice;
        remaining[current] -= slice;
        /* Enqueue arrivals before putting an unfinished job at the tail. */
        enqueueArrivals(plist, remaining, admitted, n, time, ready, &tail, &count);
        if (remaining[current] > 0) {
            ready[tail] = current;
            tail = (tail + 1) % n;
            count++;
        } else {
            plist[current].wt = time - plist[current].art - plist[current].bt;
            completed++;
        }
    }
    free(remaining);
    free(admitted);
    free(ready);
}

/** SRTF (preemptive SJF): select the shortest ready remaining burst each tick.
 *  An equal-length arrival does not preempt the currently running process.
 */
void findWaitingTimeSJF(ProcessType plist[], int n)
{
    if (n <= 0) return;
    int *remaining = malloc((size_t)n * sizeof(int));
    if (!remaining) exit(EXIT_FAILURE);
    int finished = 0, time = 0, running = -1;
    for (int i = 0; i < n; ++i) {
        remaining[i] = plist[i].bt;
        plist[i].wt = 0;
        if (remaining[i] <= 0) finished++;
    }
    while (finished < n) {
        int best = -1;
        int nextArrival = INT_MAX;
        for (int i = 0; i < n; ++i) {
            if (remaining[i] <= 0) continue;
            if (plist[i].art > time) {
                /* Not ready yet: remember when an idle CPU can wake up. */
                if (plist[i].art < nextArrival) nextArrival = plist[i].art;
                continue;
            }
            if (best == -1 || remaining[i] < remaining[best] ||
                (remaining[i] == remaining[best] &&
                 (i == running ||
                  (best != running && plist[i].art > plist[best].art))))
                best = i;
        }
        if (best == -1) {
            if (nextArrival == INT_MAX) break;
            time = nextArrival;
            running = -1;
            continue;
        }
        running = best;
        /* One simulated CPU tick, followed by completion accounting. */
        remaining[best]--;
        time++;
        if (remaining[best] == 0) {
            plist[best].wt = time - plist[best].art - plist[best].bt;
            if (plist[best].wt < 0) plist[best].wt = 0;
            finished++;
            running = -1;
        }
    }
    free(remaining);
}

/** Non-preemptive FCFS, including idle intervals between arrivals. */
void findWaitingTime(ProcessType plist[], int n)
{
    int clock = 0;
    for (int i = 0; i < n; ++i) {
        if (clock < plist[i].art) clock = plist[i].art;
        plist[i].wt = clock - plist[i].art;
        clock += plist[i].bt;
    }
}

/** Turnaround time equals time spent waiting plus CPU burst time. */
void findTurnAroundTime(ProcessType plist[], int n)
{
    for (int i = 0; i < n; ++i)
        plist[i].tat = plist[i].bt + plist[i].wt;
}

/** Highest numeric priority first, as in the provided reference executable.
 *  Among equal priorities, earlier arrivals and then lower process IDs win.
 *  Explicit tie keys make qsort deterministic (qsort itself is not stable).
 */
int my_comparer(const void *left, const void *right)
{
    const ProcessType *a = left, *b = right;
    if (a->pri < b->pri) return 1;
    if (a->pri > b->pri) return -1;
    if (a->art < b->art) return -1;
    if (a->art > b->art) return 1;
    if (a->pid < b->pid) return -1;
    if (a->pid > b->pid) return 1;
    return 0;
}

/** Run FCFS and print the algorithm header. */
void findavgTimeFCFS(ProcessType plist[], int n)
{
    findWaitingTime(plist, n);
    findTurnAroundTime(plist, n);
    printf("\n*********\nFCFS\n");
}

/** Run the assignment's preemptive SJF/SRTF variant. */
void findavgTimeSJF(ProcessType plist[], int n)
{
    findWaitingTimeSJF(plist, n);
    findTurnAroundTime(plist, n);
    printf("\n*********\nSJF\n");
}

/** Run RR with the selected quantum and print its header. */
void findavgTimeRR(ProcessType plist[], int n, int quantum)
{
    findWaitingTimeRR(plist, n, quantum);
    findTurnAroundTime(plist, n);
    printf("\n*********\nRR Quantum = %d\n", quantum);
}

/** Sort once by priority and apply non-preemptive FCFS waiting times. */
void findavgTimePriority(ProcessType plist[], int n)
{
    qsort(plist, (size_t)n, sizeof(ProcessType), my_comparer);
    findWaitingTime(plist, n);
    findTurnAroundTime(plist, n);
    printf("\n*********\nPriority\n");
}

/** Print per-process and average waiting/turnaround metrics. */
void printMetrics(ProcessType plist[], int n)
{
    int total_wt = 0, total_tat = 0;
    printf("\tProcesses\tBurst time\tWaiting time\tTurn around time\n");
    for (int i = 0; i < n; ++i) {
        total_wt += plist[i].wt;
        total_tat += plist[i].tat;
        printf("\t%d\t\t%d\t\t%d\t\t%d\n", plist[i].pid, plist[i].bt, plist[i].wt, plist[i].tat);
    }
    printf("\nAverage waiting time = %.2f", n ? (float)total_wt / n : 0.0);
    printf("\nAverage turn around time = %.2f\n", n ? (float)total_tat / n : 0.0);
}

/** Validate all six integer columns before calling the supplied file parser.
 *  The starter parser uses feof() and does not check fscanf() failures,
 *  so malformed or blank lines need to be caught before it runs.
 */
static int validInput(FILE *input, int *rows)
{
    char line[256], extra;
    int pid, bt, art, wt, tat, pri;
    *rows = 0;
    while (fgets(line, sizeof(line), input)) {
        if (sscanf(line, "%d %d %d %d %d %d %c",
                   &pid, &bt, &art, &wt, &tat, &pri, &extra) != 6 ||
            bt < 0 || art < 0)
            return 0;
        (*rows)++;
    }
    if (ferror(input)) return 0;
    rewind(input);
    return 1;
}

/** Open a file, reject malformed input, and create a fresh process list. */
ProcessType *initProc(char *filename, int *n)
{
    FILE *input_file = fopen(filename, "r");
    if (!input_file) {
        fprintf(stderr, "Error: Invalid filepath\n");
        exit(EXIT_FAILURE);
    }
    int rows = 0;
    if (!validInput(input_file, &rows)) {
        fprintf(stderr, "Error: Expected six integers per process and nonnegative burst/arrival times\n");
        fclose(input_file);
        exit(EXIT_FAILURE);
    }
    if (rows == 0) {
        fclose(input_file);
        *n = 0;
        return NULL;
    }
    ProcessType *plist = parse_file(input_file, n);
    fclose(input_file);
    if (!plist) {
        fprintf(stderr, "Error: Failed to allocate process list\n");
        exit(EXIT_FAILURE);
    }
    return plist;
}

/** Re-load input for each policy so sorting/metrics cannot affect later runs. */
int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage: ./schedsim <input-file-path>\n");
        return 1;
    }
    int n = 0, quantum = 2;
    ProcessType *proc_list;
    proc_list = initProc(argv[1], &n);
    findavgTimeFCFS(proc_list, n);
    printMetrics(proc_list, n);
    free(proc_list);
    n = 0;
    proc_list = initProc(argv[1], &n);
    findavgTimeSJF(proc_list, n);
    printMetrics(proc_list, n);
    free(proc_list);
    n = 0;
    proc_list = initProc(argv[1], &n);
    findavgTimePriority(proc_list, n);
    printMetrics(proc_list, n);
    free(proc_list);
    n = 0;
    proc_list = initProc(argv[1], &n);
    findavgTimeRR(proc_list, n, quantum);
    printMetrics(proc_list, n);
    free(proc_list);
    return 0;
}
