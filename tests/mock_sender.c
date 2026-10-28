#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "common.h"
#include "ipc_queue.h"

int main(int argc, char *argv[]) {
    int count = 3;
    if (argc > 1) {
        count = atoi(argv[1]);
        if (count <= 0) count = 3;
    }

    printf("[Mock Sender] Connecting to queue %s...\n", QUEUE_NAME);
    int mq = -1;
    // Retry a few times in case the parent process is still starting up
    for (int attempts = 0; attempts < 10; attempts++) {
        mq = init_queue_writer();
        if (mq >= 0) break;
        usleep(100000); // 100ms
    }

    if (mq < 0) {
        fprintf(stderr, "[Mock Sender] Failed to connect to queue writer\n");
        return 1;
    }

    printf("[Mock Sender] Connected! Sending %d sample observations...\n", count);

    const char *ages[] = {"adult", "young", "old"};

    for (int i = 0; i < count; i++) {
        ObservationData obs;
        memset(&obs, 0, sizeof(obs));

        snprintf(obs.observing_personname, sizeof(obs.observing_personname), "Mock_Observer_%d", i + 1);
        snprintf(obs.territory_name, sizeof(obs.territory_name), "%s", LOCATIONS[i % NUM_LOCATIONS]);
        obs.bison_number = (i + 1) * 2;
        memcpy(obs.bison_age, ages[i % 3], strlen(ages[i % 3]));
        snprintf(obs.observation_time, sizeof(obs.observation_time), "2026-10-28 15:%02d:00", i);

        if (send_observation(mq, &obs) == 0) {
            printf("[Mock Sender] Sent: [%s] %s @ %s, count=%d, age=%.*s\n",
                obs.observation_time, obs.observing_personname, obs.territory_name,
                obs.bison_number, (int)sizeof(obs.bison_age), obs.bison_age);
        } else {
            fprintf(stderr, "[Mock Sender] Failed to send observation %d\n", i + 1);
        }
        usleep(50000); // 50ms between sends
    }

    close(mq);
    printf("[Mock Sender] Finished sending all samples.\n");
    return 0;
}
