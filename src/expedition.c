// ВРЕМЕННЫЕ ЗАГЛУШКИ ДЛЯ ОТЛАДКИ МЕЙНА

#include <stdio.h>
#include <unistd.h>
#include "common.h"

void run_expedition(int read_pipe_fd, const char* observer_name) {
    char location_buf[MAX_LOC_LEN] = { 0 };

    // Читаем локацию из pipe, отправленную из main.c
    read(read_pipe_fd, location_buf, sizeof(location_buf));
    close(read_pipe_fd);

    printf("[Expedition Stub] %s заступил на локацию: %s\n", observer_name, location_buf);
}