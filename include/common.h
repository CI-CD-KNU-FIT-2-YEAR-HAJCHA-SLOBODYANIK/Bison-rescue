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
    "Ternopil Mountains",
    "Zhytomyr Federation",
    "Benches Under Great Wall",
    "Moon",
    "Zimbabwe",
    "DPRK-controlled South Korean Territory",
    "Africa (North Pole)"
};
#define NUM_LOCATIONS (sizeof(LOCATIONS) / sizeof(LOCATIONS[0]))

// Observation report structure
typedef struct {
    char observing_personname[64];
    char territory_name[64];
    int bison_number;
    char bison_age[16];                         // "young", "adult", "old"
    char observation_time[32];                  // "YYYY-MM-DD HH:MM:SS"
} ObservationData;

// Message queue wrapper functions (implemented in src/ipc_queue.c)
int init_queue_writer(void);
int init_queue_reader(void);
int send_observation(int mq_fd, const ObservationData *data);
int receive_observation(int mq_fd, ObservationData *data);
void cleanup_queue(int mq_fd);

// Backward compatibility wrappers for Team Member 1 prototypes
mqd_t ipc_server_init(void);
mqd_t ipc_client_init(void);
int ipc_send_observation(mqd_t mqdes, const ObservationData *data);
int ipc_receive_observation(mqd_t mqdes, ObservationData *data);
void ipc_close(mqd_t mqdes);
void ipc_unlink(void);

// Child process entry point (implemented in src/expedition.c)
void run_expedition(int read_pipe_fd, const char *observer_name);

#endif // COMMON_H
