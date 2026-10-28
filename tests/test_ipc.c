#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include "common.h"
#include "ipc_queue.h"

static void set_bison_age(char *dest, size_t dest_sz, const char *src) {
    size_t len = strlen(src);
    if (len > dest_sz) len = dest_sz;
    memcpy(dest, src, len);
}

int main(void) {
    printf("=== [TEST] Starting IPC Queue Unit Tests ===\n");

    // 1. Initialize reader queue
    int reader_fd = init_queue_reader();
    assert(reader_fd >= 0 && "init_queue_reader failed");
    printf("[PASS] init_queue_reader created queue successfully (fd=%d)\n", reader_fd);

    // 2. Verify non-blocking read on empty queue
    ObservationData dummy;
    int rc = receive_observation(reader_fd, &dummy);
    assert(rc == -1 && "receive_observation on empty queue should return -1 (EAGAIN)");
    printf("[PASS] receive_observation on empty queue correctly returned -1\n");

    // 3. Initialize writer queue
    int writer_fd = init_queue_writer();
    assert(writer_fd >= 0 && "init_queue_writer failed");
    printf("[PASS] init_queue_writer opened queue successfully (fd=%d)\n", writer_fd);

    // 4. Send sample ObservationData
    ObservationData sample1;
    memset(&sample1, 0, sizeof(sample1));
    strncpy(sample1.observing_personname, "Тестовий_Еколог_1", sizeof(sample1.observing_personname) - 1);
    strncpy(sample1.territory_name, "Гори Тернополя", sizeof(sample1.territory_name) - 1);
    sample1.bison_number = 7;
    set_bison_age(sample1.bison_age, sizeof(sample1.bison_age), "дорослий");
    strncpy(sample1.observation_time, "2026-10-28 14:30:00", sizeof(sample1.observation_time) - 1);

    rc = send_observation(writer_fd, &sample1);
    assert(rc == 0 && "send_observation failed for sample1");
    printf("[PASS] send_observation succeeded for sample1\n");

    ObservationData sample2;
    memset(&sample2, 0, sizeof(sample2));
    strncpy(sample2.observing_personname, "Тестовий_Еколог_2", sizeof(sample2.observing_personname) - 1);
    strncpy(sample2.territory_name, "Житомирська федерація", sizeof(sample2.territory_name) - 1);
    sample2.bison_number = 3;
    set_bison_age(sample2.bison_age, sizeof(sample2.bison_age), "молодий");
    strncpy(sample2.observation_time, "2026-10-28 14:31:15", sizeof(sample2.observation_time) - 1);

    rc = send_observation(writer_fd, &sample2);
    assert(rc == 0 && "send_observation failed for sample2");
    printf("[PASS] send_observation succeeded for sample2\n");

    // 5. Receive observations and verify integrity
    ObservationData received1;
    memset(&received1, 0, sizeof(received1));
    rc = receive_observation(reader_fd, &received1);
    assert(rc == 0 && "receive_observation failed for sample1");
    assert(strcmp(received1.observing_personname, sample1.observing_personname) == 0);
    assert(strcmp(received1.territory_name, sample1.territory_name) == 0);
    assert(received1.bison_number == sample1.bison_number);
    assert(memcmp(received1.bison_age, sample1.bison_age, sizeof(sample1.bison_age)) == 0);
    assert(strcmp(received1.observation_time, sample1.observation_time) == 0);
    printf("[PASS] received1 matched sample1 data exactly\n");

    ObservationData received2;
    memset(&received2, 0, sizeof(received2));
    rc = receive_observation(reader_fd, &received2);
    assert(rc == 0 && "receive_observation failed for sample2");
    assert(strcmp(received2.observing_personname, sample2.observing_personname) == 0);
    assert(strcmp(received2.territory_name, sample2.territory_name) == 0);
    assert(received2.bison_number == sample2.bison_number);
    assert(memcmp(received2.bison_age, sample2.bison_age, sizeof(sample2.bison_age)) == 0);
    assert(strcmp(received2.observation_time, sample2.observation_time) == 0);
    printf("[PASS] received2 matched sample2 data exactly\n");

    // 6. Verify queue is now empty again
    rc = receive_observation(reader_fd, &dummy);
    assert(rc == -1 && "Queue should be empty after reading all items");
    printf("[PASS] Queue is empty after reading all items\n");

    // 7. Cleanup
    close(writer_fd);
    cleanup_queue(reader_fd);
    printf("[PASS] cleanup_queue executed successfully\n");

    printf("=== [TEST] All IPC Queue Unit Tests PASSED! ===\n");
    return 0;
}
