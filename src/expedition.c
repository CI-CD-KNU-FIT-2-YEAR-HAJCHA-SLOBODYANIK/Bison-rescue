#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include "common.h"
#include "ipc_queue.h"

static volatile sig_atomic_t g_running = 1;

static void handle_sigterm(int sig) {
    (void)sig;
    g_running = 0;
}

void run_expedition(int read_pipe_fd, const char* observer_name) {
    char location_buf[MAX_LOC_LEN] = { 0 };

    // Read assigned location from pipe sent by main.c
    ssize_t bytes = read(read_pipe_fd, location_buf, sizeof(location_buf) - 1);
    close(read_pipe_fd);

    if (bytes <= 0) {
        snprintf(location_buf, sizeof(location_buf), "Unknown territory");
    }

    printf("[Expedition] %s deployed to location: %s\n", observer_name, location_buf);

    signal(SIGTERM, handle_sigterm);

    int mq = init_queue_writer();
    if (mq < 0) {
        perror("[Expedition] Failed to open queue writer");
        return;
    }

    const char *ages[] = {"adult", "young", "old"};
    unsigned int seed = (unsigned int)(time(NULL) ^ (getpid() << 4));
    int obs_counter = 0;

    while (g_running) {
        // Observation generation interval (1-2 seconds)
        sleep(1);
        if (!g_running) break;

        obs_counter++;
        ObservationData obs;
        memset(&obs, 0, sizeof(obs));

        strncpy(obs.observing_personname, observer_name, sizeof(obs.observing_personname) - 1);
        strncpy(obs.territory_name, location_buf, sizeof(obs.territory_name) - 1);
        obs.bison_number = (rand_r(&seed) % 8) + 1;

        const char *chosen_age = ages[rand_r(&seed) % 3];
        size_t age_len = strlen(chosen_age);
        if (age_len > sizeof(obs.bison_age)) age_len = sizeof(obs.bison_age);
        memcpy(obs.bison_age, chosen_age, age_len);

        time_t now = time(NULL);
        struct tm tm_buf;
        localtime_r(&now, &tm_buf);
        strftime(obs.observation_time, sizeof(obs.observation_time), "%Y-%m-%d %H:%M:%S", &tm_buf);

        if (send_observation(mq, &obs) == 0) {
            printf("[Expedition] %s sent report #%d from %s: %d bison(s)\n",
                observer_name, obs_counter, location_buf, obs.bison_number);
        }
    }

    close(mq);
    printf("[Expedition] %s completed work.\n", observer_name);
}
