#ifndef COMMON_H
#define COMMON_H

#include <time.h>
#include <sys/types.h>

#define QUEUE_NAME "/bison_queue"
#define MAX_MSG_SIZE 256

//Структура спостереження
typedef struct {
    char observing_personname[64];
    char territory_name[64];
    int bison_number;
    char bison_age[16];                         //"молодий", "дорослий", "старий"
    char observation_time[32];                  //"YYYY-MM-DD HH:MM:SS"
} ObservationData;

#endif