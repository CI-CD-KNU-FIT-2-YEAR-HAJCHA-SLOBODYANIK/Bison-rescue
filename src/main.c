#include <stdio.h> 
#include <stdlib.h>  
#include <string.h>
#include <unistd.h> 
#include <signal.h> 
#include <time.h>   
#include <sys/wait.h>  
#include "common.h"   
#include "ipc_queue.h"

int main(int argc, char* argv[]) {
    // Default variable initialization BEFORE the getopt loop
    int n_expeditions = 0;
    int session_time = 0;
    int opt;

    while ((opt = getopt(argc, argv, "n:t:")) != -1) {
        switch (opt) {
            case 'n':
                n_expeditions = atoi(optarg);
                break;
            case 't':
                session_time = atoi(optarg);
                break;
            default:
                fprintf(stderr, "Usage: %s -n <N_expeditions> -t <T_session_time_sec>\n", argv[0]);
                return 1;
        }
    }

    if (n_expeditions <= 0 || session_time <= 0) {
        fprintf(stderr, "Usage: %s -n <N_expeditions> -t <T_session_time_sec>\n", argv[0]);
        return 1;
    }

    printf("[Main] Starting system: %d expeditions, session duration %d sec\n", n_expeditions, session_time);

    int mq = init_queue_reader();  // Initialize message queue for reading
    if (mq == -1) {
        fprintf(stderr, "[Main] Error initializing message queue\n");
        return 1;
    }

    FILE* csv = fopen("results/observations.csv", "w");
    if (!csv) {
        perror("[Main] Error opening results/observations.csv");
        cleanup_queue(mq);
        return 1;
    }

    fprintf(csv, "Timestamp,Territory,Observer,Count,AgeGroup\n");
    fflush(csv);

    pid_t* pids = malloc(sizeof(pid_t) * n_expeditions);
    if (!pids) {
        perror("[Main] malloc pids error");
        fclose(csv);
        cleanup_queue(mq);
        return 1;
    }

    for (int i = 0; i < n_expeditions; i++) {
        int pipefd[2]; // pipefd[0] read | pipefd[1] write

        if (pipe(pipefd) == -1) {
            perror("[Main] pipe error");
            free(pids);
            fclose(csv);
            cleanup_queue(mq);
            exit(1);
        }
        pid_t pid = fork();

        if (pid == 0) { // Child process
            
            // Child will not write to pipe, close write descriptor
            close(pipefd[1]);

            char name_buf[64];
            snprintf(name_buf, sizeof(name_buf), "Observer_%d", i + 1);

            run_expedition(pipefd[0], name_buf);

            exit(0);
        }
        else if (pid > 0) {  // Parent process
            
            // Parent will not read from pipe, close read descriptor
            close(pipefd[0]);

            pids[i] = pid;

            const char* loc = LOCATIONS[i % NUM_LOCATIONS];

            if (write(pipefd[1], loc, strlen(loc) + 1) == -1) {
                perror("[Main] write to pipe error");
            }

            // Close write descriptor
            close(pipefd[1]);

        }
        else {
            perror("[Main] fork error");
            free(pids);
            fclose(csv);
            cleanup_queue(mq);
            exit(1);
        }
    }

    time_t start_time = time(NULL);
    ObservationData obs;

    while (time(NULL) - start_time < session_time) {  // Run for session_time seconds

        if (receive_observation(mq, &obs) == 0) {

            printf("[Main] Entry from %s (%s): %d bison(s), %.*s, %s\n",
                obs.observing_personname, obs.territory_name, obs.bison_number,
                (int)sizeof(obs.bison_age), obs.bison_age, obs.observation_time);

            fprintf(csv, "%s,%s,%s,%d,%.*s\n",
                obs.observation_time,
                obs.territory_name,
                obs.observing_personname,
                obs.bison_number,
                (int)sizeof(obs.bison_age),
                obs.bison_age);

            fflush(csv);
        }
        else {
            usleep(100000); // 100 ms wait
        }
    }

    printf("[Main] Session time elapsed. Terminating processes...\n");

    for (int i = 0; i < n_expeditions; i++) {
        kill(pids[i], SIGTERM);
        waitpid(pids[i], NULL, 0);
    }
    free(pids);

    // Drain any remaining messages in the queue before shutdown
    while (receive_observation(mq, &obs) == 0) {
        printf("[Main] Final entry from %s (%s): %d bison(s)\n",
            obs.observing_personname, obs.territory_name, obs.bison_number);

        fprintf(csv, "%s,%s,%s,%d,%.*s\n",
            obs.observation_time,
            obs.territory_name,
            obs.observing_personname,
            obs.bison_number,
            (int)sizeof(obs.bison_age),
            obs.bison_age);

        fflush(csv);
    }

    fclose(csv);
    cleanup_queue(mq);

    printf("[Main] Execution completed. Data saved to results/observations.csv\n");
    return 0;
}
