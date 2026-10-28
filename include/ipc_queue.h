#ifndef IPC_QUEUE_H
#define IPC_QUEUE_H

#include "common.h"

int init_queue_writer(void);
int init_queue_reader(void);
int send_observation(int mq_fd, const ObservationData *data);
int receive_observation(int mq_fd, ObservationData *data);
void cleanup_queue(int mq_fd);

#endif // IPC_QUEUE_H
