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
    // Реєструємо обробник сигналу завершення
    signal(SIGTERM, handle_sigterm);

    //usleep(50000); // 50 мс йдемо на місце
    kill(getppid(), SIGUSR1);  // Сигнал батьківському процесу про прибуття

    char location_buf[MAX_LOC_LEN] = { 0 };

    // Зчитуємо призначену територію з пайпу
    ssize_t bytes = read(read_pipe_fd, location_buf, sizeof(location_buf) - 1);
    close(read_pipe_fd);

    if (bytes <= 0) {
        snprintf(location_buf, sizeof(location_buf), "Невідома територія");
    }

    kill(getppid(), SIGUSR2);  // Сигнал батьківському процесу про готовність до роботи

    printf("[Експедиція] %s прибув на локацію: %s. Готовий до спостережень!\n", observer_name, location_buf);

    // Реєструємо обробник сигналу завершення
    signal(SIGTERM, handle_sigterm);

    // Відкриваємо чергу повідомлень для запису
    int mq = init_queue_writer();
    if (mq < 0) {
        perror("[Експедиція] Помилка відкриття черги повідомлень");
        return;
    }

    // Створення локального журналу бекапу
    char log_filename[128];
    snprintf(log_filename, sizeof(log_filename), "results/%s_log.txt", observer_name);
    FILE *local_log = fopen(log_filename, "w");
    if (local_log) {
        fprintf(local_log, "=== Локальний журнал: %s ===\n", observer_name);
        fprintf(local_log, "Локація: %s\n", location_buf);
        fprintf(local_log, "-----------------------------------\n");
        fflush(local_log);
    }

    const char *ages[] = {"adult", "young", "old"};
    unsigned int seed = (unsigned int)(time(NULL) ^ (getpid() << 4));

    // Змінні для внутрішньої аналітики
    int obs_counter = 0;
    int total_bisons_seen = 0;

    // Головний цикл життя процесу
    while (g_running) {
        // Випадкова затримка 2-5 секунд
        int delay = (rand_r(&seed) % 4) + 2;
        sleep(delay);

        if (!g_running) break;

        obs_counter++;
        ObservationData obs;
        memset(&obs, 0, sizeof(obs));

        strncpy(obs.observing_personname, observer_name, sizeof(obs.observing_personname) - 1);
        strncpy(obs.territory_name, location_buf, sizeof(obs.territory_name) - 1);

        // Випадкова кількість зубрів та вік
        obs.bison_number = (rand_r(&seed) % 8) + 1;
        total_bisons_seen += obs.bison_number;

        const char *chosen_age = ages[rand_r(&seed) % 3];
        size_t age_len = strlen(chosen_age);
        if (age_len > sizeof(obs.bison_age)) age_len = sizeof(obs.bison_age);
        memcpy(obs.bison_age, chosen_age, age_len);

        // Фіксуємо поточний системний час
        time_t now = time(NULL);
        struct tm tm_buf;
        localtime_r(&now, &tm_buf);
        strftime(obs.observation_time, sizeof(obs.observation_time), "%Y-%m-%d %H:%M:%S", &tm_buf);

        // Відправка в головну чергу та вивід у консоль
        if (send_observation(mq, &obs) == 0) {
            printf("[Експедиція] %s передає звіт #%d з локації '%s': помічено %d зубр(ів)\n",
                observer_name, obs_counter, location_buf, obs.bison_number);

            // Резервний запис у локальний файл-бекап
            if (local_log) {
                fprintf(local_log, "[%s] Звіт #%d: Помічено %d зубрів (категорія: %s)\n",
                        obs.observation_time, obs_counter, obs.bison_number, obs.bison_age);
                fflush(local_log);
            }
        }
    }

    // Закриваємо чергу
    close(mq);

    // Завершення локального журналу
    if (local_log) {
        fprintf(local_log, "-----------------------------------\n");
        fprintf(local_log, "Експедицію завершено. Всього звітів: %d. Всього зубрів знайдено: %d.\n", obs_counter, total_bisons_seen);
        fclose(local_log);
    }

    // Фінальний вивід зі статистикою
    printf("[Експедиція] %s втомлена, повертаємось на базу! (Звітів: %d, Загалом знайдено зубрів: %d)\n",
            observer_name, obs_counter, total_bisons_seen);
}
