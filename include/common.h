#ifndef COMMON_H
#define COMMON_H

#include <sys/types.h>
#include <mqueue.h>

#define QUEUE_NAME "/bison_queue"
#define MAX_MSG_SIZE 256
#define MAX_LOC_LEN 64
#define QUEUE_PERMS 0660
#define MAX_MESSAGES 10

static const char* LOCATIONS[] __attribute__((unused)) = {
    "Горы Тернополя",
    "Житомирская федерация",
    "Лавочки под китайской стеной",
    "Луна",
    "Зимбабве",
    "КНДР-подконтрольная территория Южной Кореи",
    "Африка (Северный полюс)"
};
#define NUM_LOCATIONS (sizeof(LOCATIONS) / sizeof(LOCATIONS[0]))

// Структура отчета наблюдения
typedef struct {
    char observing_personname[64];
    char territory_name[64];
    int bison_number;
    char bison_age[16];                         //"молодий", "дорослий", "старий"
    char observation_time[32];                  //"YYYY-MM-DD HH:MM:SS"
} ObservationData;

// Обёртки работы с очередью сообщений (реализация в src/ipc_queue.c)
int init_queue_writer(void);
int init_queue_reader(void);
int send_observation(int mq_fd, const ObservationData *data);
int receive_observation(int mq_fd, ObservationData *data);
void cleanup_queue(int mq_fd);

// Совместимость с прототипами Team Member 1
mqd_t ipc_server_init(void);
mqd_t ipc_client_init(void);
int ipc_send_observation(mqd_t mqdes, const ObservationData *data);
int ipc_receive_observation(mqd_t mqdes, ObservationData *data);
void ipc_close(mqd_t mqdes);
void ipc_unlink(void);

// Точка входа дочернего процесса (реализация в src/expedition.c)
void run_expedition(int read_pipe_fd, const char *observer_name);

#endif // COMMON_H
