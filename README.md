# Lab 6: Scheduling Simulator

Howard University Operating Systems lab implementing four CPU scheduling algorithms in C:

- First Come, First Served (FCFS)
- Preemptive Shortest Job First / Shortest Remaining Time First (SRTF)
- Priority scheduling
- Round Robin (time quantum = 2)

The simulator reports each process's burst time, waiting time, turnaround time, and the average waiting and turnaround times.

## Build and run

```bash
make
./schedsim input0.txt
./schedsim input1.txt
./schedsim input2.txt
```

You can also compile directly with `gcc -Wall -Wextra -std=c11 -o schedsim schedsim.c util.c`.

The input format is one process per line, with six integer columns:
`PID burst_time arrival_time waiting_time turnaround_time priority`.

The implementation uses the larger-number-first priority convention shown by the course's reference executable. Round Robin maintains an arrival-aware ready queue, and SRTF chooses the shortest remaining burst among processes that have arrived.

The repository includes source files and sample input files, but omits Codio-specific metadata and the supplied reference executable binaries.
