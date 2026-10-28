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

    // Читаем локацию из pipe, отправленную из main.c
    ssize_t bytes = read(read_pipe_fd, location_buf, sizeof(location_buf) - 1);
    close(read_pipe_fd);

    if (bytes <= 0) {
        snprintf(location_buf, sizeof(location_buf), "Невідома територія");
    }

    printf("[Expedition] %s заступив на локацію: %s\n", observer_name, location_buf);

    signal(SIGTERM, handle_sigterm);

    int mq = init_queue_writer();
    if (mq < 0) {
        perror("[Expedition] Failed to open queue writer");
        return;
    }

    const char *ages[] = {"дорослий", "молодий", "старий"};
    unsigned int seed = (unsigned int)(time(NULL) ^ (getpid() << 4));
    int obs_counter = 0;

    while (g_running) {
        // Интервал генерации наблюдений (1-2 секунды)
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
            printf("[Expedition] %s надіслав звіт #%d з %s: %d зубр(ів)\n",
                observer_name, obs_counter, location_buf, obs.bison_number);
        }
    }

    close(mq);
    printf("[Expedition] %s завершив роботу.\n", observer_name);
}
