// ВРЕМЕННЫЕ ЗАГЛУШКИ ДЛЯ ОТЛАДКИ МЕЙНА

#include <stdio.h>
#include "common.h"

mqd_t ipc_server_init(void) {
    printf("[IPC Stub] Server queue initialized\n");
    return (mqd_t)1;
}

mqd_t ipc_client_init(void) {
    return (mqd_t)1;
}

int ipc_send_observation(mqd_t mqdes, const ObservationData* data) {
    (void)mqdes;
    (void)data;
    return 0;
}

int ipc_receive_observation(mqd_t mqdes, ObservationData* data) {
    (void)mqdes;
    (void)data;
    // Возвращаем -1, чтобы главный цикл не зацикливался на пустых данных
    return -1;
}

void ipc_close(mqd_t mqdes) {
    (void)mqdes;
}

void ipc_unlink(void) {
    printf("[IPC Stub] Queue unlinked\n");
}