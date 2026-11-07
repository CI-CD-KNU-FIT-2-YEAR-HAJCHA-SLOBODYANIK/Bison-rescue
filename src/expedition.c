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

    // Головний цикл життя процесу
    while (g_running) {
        sleep(1); // Тимчасова затримка
    }

    printf("[Експедиція] %s втомлена, повертаємось на базу.\n", observer_name);
}
