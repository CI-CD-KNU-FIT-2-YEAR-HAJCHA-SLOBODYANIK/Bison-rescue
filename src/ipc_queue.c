#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <errno.h>
#include "ipc_queue.h"

int init_queue_reader(void) {
    struct mq_attr attr;
    memset(&attr, 0, sizeof(attr));
    attr.mq_flags = O_NONBLOCK;
    attr.mq_maxmsg = MAX_MESSAGES;
    attr.mq_msgsize = sizeof(ObservationData);
    attr.mq_curmsgs = 0;

    // Unlink old queue if exists to ensure clean attributes
    mq_unlink(QUEUE_NAME);

    mqd_t mq = mq_open(QUEUE_NAME, O_CREAT | O_RDONLY | O_NONBLOCK, QUEUE_PERMS, &attr);
    if (mq == (mqd_t)-1) {
        perror("[IPC] mq_open reader failed");
        return -1;
    }
    return (int)mq;
}

int init_queue_writer(void) {
    mqd_t mq = mq_open(QUEUE_NAME, O_WRONLY);
    if (mq == (mqd_t)-1) {
        perror("[IPC] mq_open writer failed");
        return -1;
    }
    return (int)mq;
}

int send_observation(int mq_fd, const ObservationData *data) {
    if (!data) {
        return -1;
    }
    if (mq_send((mqd_t)mq_fd, (const char *)data, sizeof(ObservationData), 0) == -1) {
        perror("[IPC] mq_send failed");
        return -1;
    }
    return 0;
}

int receive_observation(int mq_fd, ObservationData *data) {
    if (!data) {
        return -1;
    }
    ssize_t bytes_read = mq_receive((mqd_t)mq_fd, (char *)data, sizeof(ObservationData), NULL);
    if (bytes_read == -1) {
        if (errno == EAGAIN) {
            // Queue is empty (non-blocking)
            return -1;
        }
        perror("[IPC] mq_receive failed");
        return -1;
    }
    return 0;
}

void cleanup_queue(int mq_fd) {
    if (mq_fd >= 0) {
        mq_close((mqd_t)mq_fd);
    }
    mq_unlink(QUEUE_NAME);
}

// Backward compatibility wrappers for Team Member 1 prototypes
mqd_t ipc_server_init(void) {
    return (mqd_t)init_queue_reader();
}

mqd_t ipc_client_init(void) {
    return (mqd_t)init_queue_writer();
}

int ipc_send_observation(mqd_t mqdes, const ObservationData *data) {
    return send_observation((int)mqdes, data);
}

int ipc_receive_observation(mqd_t mqdes, ObservationData *data) {
    return receive_observation((int)mqdes, data);
}

void ipc_close(mqd_t mqdes) {
    if (mqdes >= 0) {
        mq_close(mqdes);
    }
}

void ipc_unlink(void) {
    mq_unlink(QUEUE_NAME);
}
