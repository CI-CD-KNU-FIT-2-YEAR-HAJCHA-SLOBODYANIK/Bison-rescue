#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include "common.h"
#include "ipc_queue.h"

static volatile sig_atomic_t g_running = 1;

// Обробник сигналу від батьківського процесу
static void handle_sigterm(int sig) {
    (void)sig;
    g_running = 0;
}

void run_expedition(int read_pipe_fd, const char* observer_name) {
    char location_buf[MAX_LOC_LEN] = { 0 };

    // Зчитуємо призначену територію з пайпу
    ssize_t bytes = read(read_pipe_fd, location_buf, sizeof(location_buf) - 1);
    close(read_pipe_fd);

    if (bytes <= 0) {
        snprintf(location_buf, sizeof(location_buf), "Невідома територія");
    }

    printf("[Експедиція] %s прибув на локацію: %s. Готовий до спостережень!\n", observer_name, location_buf);

    // Реєструємо обробник сигналу завершення
    signal(SIGTERM, handle_sigterm);

    const char *ages[] = {"adult", "young", "old"};
    unsigned int seed = (unsigned int)(time(NULL) ^ (getpid() << 4));

    // Головний цикл життя процесу
    while (g_running) {
        // Випадкова затримка 2-5 секунд (ТЗ)
        int delay = (rand_r(&seed) % 4) + 2;
        sleep(delay);

        if (!g_running) break;

        ObservationData obs;
        memset(&obs, 0, sizeof(obs));

        strncpy(obs.observing_personname, observer_name, sizeof(obs.observing_personname) - 1);
        strncpy(obs.territory_name, location_buf, sizeof(obs.territory_name) - 1);

        // Випадкова кількість зубрів та вік
        obs.bison_number = (rand_r(&seed) % 8) + 1;
        const char *chosen_age = ages[rand_r(&seed) % 3];
        size_t age_len = strlen(chosen_age);
        if (age_len > sizeof(obs.bison_age)) age_len = sizeof(obs.bison_age);
        memcpy(obs.bison_age, chosen_age, age_len);

        // Фіксуємо поточний системний час
        time_t now = time(NULL);
        struct tm tm_buf;
        localtime_r(&now, &tm_buf);
        strftime(obs.observation_time, sizeof(obs.observation_time), "%Y-%m-%d %H:%M:%S", &tm_buf);

        // Поки що просто виводимо в консоль
        printf("[DEBUG] Згенеровано: %d зубрів (%s)\n", obs.bison_number, chosen_age);
    }

    printf("[Експедиція] %s втомлена, повертаємось на базу.\n", observer_name);
}
