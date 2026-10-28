### Core Parameters and IPC

* **Architectural Pattern:** Master-Worker (Main orchestrator process + $N$ expedition processes)
* **POSIX Message Queue:** `/bison_queue` (maximum message size: `256` bytes)
* **Data Exchange Mechanisms:**
  * **Initialization:** Anonymous pipes (passing territory name from parent to child process).
  * **Telemetry:** POSIX Message Queue (`mqueue.h`) for streaming structured observation reports from expeditions to parent.
* **Output Format:** CSV file (`results/observations.csv`)
* **Timestamp Format:** ISO-like local / UTC (`YYYY-MM-DD HH:MM:SS`)
* **Process Management:** Signals (`SIGTERM` for stopping worker processes) and `waitpid()` system calls.

---

### 1. Build and Run

The project is built using `Makefile`. Launching the executable requires CLI flags:

```bash
make
./bison_rescue -n <N_expeditions> -t <T_session_time_sec>
```

#### CLI Parameters:

* `-n <N_expeditions>` (`int`): Number of spawned observer processes ($N \ge 1$).
* `-t <T_session_time_sec>` (`int`): Duration of data collection session in seconds ($T \ge 1$).

---

### 2. Output Artifacts

* **Console Output (`stdout`):** System logs from `main.c` regarding process spawning, message receipt, and shutdown.
* **Results File (`results/observations.csv`):** Streaming log of all incoming reports.

#### CSV File Format:

```csv
Timestamp,Territory,Observer,Count,AgeGroup
2026-10-28 15:00:00,Ternopil Mountains,Observer_1,3,adult
```

---

### 3. Shared Contract (`include/common.h`)

#### Report Structure (`ObservationData`)

Fixed structure for sending messages across the POSIX message queue:

```c
typedef struct {
    char observing_personname[64]; // Observer name ("Observer_1", "Observer_2"...)
    char territory_name[64];       // Assigned territory name
    int bison_number;              // Number of observed bisons
    char bison_age[16];            // Age category ("young", "adult", "old")
    char observation_time[32];     // Observation timestamp ("YYYY-MM-DD HH:MM:SS")
} ObservationData;
```

#### Datasets and IPC Interface

* `LOCATIONS[]` / `NUM_LOCATIONS` — static array of territory names.
* `int init_queue_reader(void)` — create/open `/bison_queue` for non-blocking reading in `main.c`.
* `int init_queue_writer(void)` — connect to `/bison_queue` for writing from child processes.
* `int send_observation(int mq_fd, const ObservationData *data)` — send an observation report structure.
* `int receive_observation(int mq_fd, ObservationData *data)` — receive report structure from queue (returns `0` on success, `-1` when empty or error).
* `void cleanup_queue(int mq_fd)` — close descriptor and unlink `/bison_queue` from `/dev/mqueue`.
* `void run_expedition(int read_pipe_fd, const char *observer_name)` — child process entry point.

---

### 4. Orchestrator Flow (`main.c`)

1. Parses command-line flags `-n` and `-t`.
2. Initializes the queue reader server (`init_queue_reader`).
3. Creates/clears `results/observations.csv` and writes the CSV header.
4. Spawns $N$ child processes:
   * Creates an anonymous `pipe`.
   * Calls `fork()`.
   * **Parent:** Writes the territory name `LOCATIONS[i % NUM_LOCATIONS]` to `pipefd[1]`, closes `pipefd[1]`, and records the child PID.
   * **Child:** Closes `pipefd[1]` and enters `run_expedition(pipefd[0], name_buf)`.
5. Reads incoming reports from the queue via `receive_observation()` for $T$ seconds, flushing buffers to CSV immediately (`fflush`).
6. After $T$ seconds, sends `SIGTERM` to all child PIDs, awaits termination with `waitpid()`, drains remaining queue messages, closes the CSV file, and invokes `cleanup_queue()`.

---

### 5. Expedition Requirements (`src/expedition.c`)

* **Pipe Reading:** Read the assigned territory name from `read_pipe_fd` first and immediately close the descriptor with `close(read_pipe_fd)`.
* **IPC Integration:** Connect to the queue via `init_queue_writer()` and send generated `ObservationData` structures via `send_observation()`.
* **Termination:** Intercept `SIGTERM` signals and gracefully shut down, closing the queue descriptor.

---

### 6. Queue Requirements (`src/ipc_queue.c`)

* Guarantee message size `mq_msgsize = sizeof(ObservationData)`.
* Ensure non-blocking read (`O_NONBLOCK`) on the reader side so `main.c` does not block indefinitely when the queue is empty.
