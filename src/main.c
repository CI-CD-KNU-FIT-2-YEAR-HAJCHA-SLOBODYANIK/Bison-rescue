#include <stdio.h> 
#include <stdlib.h>  
#include <string.h>
#include <unistd.h> 
#include <signal.h> 
#include <time.h>   
#include <sys/wait.h>  
#include "common.h"   
#include "ipc_queue.h"

int main(int argc, char* argv[]) {
    int n_expeditions = 0;
    int session_time = 0;
    int opt;

    while ((opt = getopt(argc, argv, "n:t:")) != -1) {
        switch (opt) {
            case 'n':
                n_expeditions = atoi(optarg);
                break;
            case 't':
                session_time = atoi(optarg);
                break;
            default:
                fprintf(stderr, "Использование: %s -n <N_экспедиций> -t <T_время_сессии_сек>\n", argv[0]);
                return 1;
        }
    }

    // Поддержка позиционных аргументов для обратной совместимости
    if (n_expeditions <= 0 && optind < argc) {
        n_expeditions = atoi(argv[optind++]);
    }
    if (session_time <= 0 && optind < argc) {
        session_time = atoi(argv[optind++]);
    }

    if (n_expeditions <= 0 || session_time <= 0) {
        fprintf(stderr, "Использование: %s -n <N_экспедиций> -t <T_время_сессии_сек>\n", argv[0]);
        return 1;
    }

    printf("[Main] Запуск системы: %d экспедиций, время работы %d сек\n", n_expeditions, session_time);

    int mq = init_queue_reader();  // инициализация очереди на чтение
    if (mq == -1) {
        fprintf(stderr, "[Main] Ошибка создания очереди сообщений\n");
        return 1;
    }

    FILE* csv = fopen("results/observations.csv", "w");
    if (!csv) {
        perror("Ошибка открытия results/observations.csv");
        cleanup_queue(mq);
        return 1;
    }

    fprintf(csv, "Timestamp,Territory,Observer,Count,AgeGroup\n");
    fflush(csv);

    pid_t* pids = malloc(sizeof(pid_t) * n_expeditions);
    if (!pids) {
        perror("malloc pids error");
        fclose(csv);
        cleanup_queue(mq);
        return 1;
    }

    for (int i = 0; i < n_expeditions; i++) {
        int pipefd[2]; // pipefd[0] чтение | pipefd[1] запись

        if (pipe(pipefd) == -1) {
            perror("pipe error");
            free(pids);
            fclose(csv);
            cleanup_queue(mq);
            exit(1);
        }
        pid_t pid = fork();

        if (pid == 0) { // дочерний процесс
            
            // Ребенок не будет писать в pipe, поэтому закрываем дескриптор записи
            close(pipefd[1]);

            char name_buf[64];
            snprintf(name_buf, sizeof(name_buf), "Наблюдатель_%d", i + 1);

            run_expedition(pipefd[0], name_buf);

            exit(0);
        }
        else if (pid > 0) {  // родительский процесс
            
            // Родитель не будет читать из pipe, поэтому закрываем дескриптор чтения
            close(pipefd[0]);

            pids[i] = pid;

            const char* loc = LOCATIONS[i % NUM_LOCATIONS];

            write(pipefd[1], loc, strlen(loc) + 1);

            // Закрываем дескриптор записи
            close(pipefd[1]);

        }
        else {
            perror("fork error");
            free(pids);
            fclose(csv);
            cleanup_queue(mq);
            exit(1);
        }
    }

    time_t start_time = time(NULL);
    ObservationData obs;

    while (time(NULL) - start_time < session_time) {  // работаем session_time секунд

        if (receive_observation(mq, &obs) == 0) {

            printf("[Main] Запис від %s (%s): %d зубр(ів), %.*s, %s\n",
                obs.observing_personname, obs.territory_name, obs.bison_number,
                (int)sizeof(obs.bison_age), obs.bison_age, obs.observation_time);

            fprintf(csv, "%s,%s,%s,%d,%.*s\n",
                obs.observation_time,
                obs.territory_name,
                obs.observing_personname,
                obs.bison_number,
                (int)sizeof(obs.bison_age),
                obs.bison_age);

            fflush(csv);
        }
        else {
            usleep(100000); // 100мс ждем
        }
    }

    printf("[Main] Время сессии истекло. Остановка процессов...\n");

    for (int i = 0; i < n_expeditions; i++) {
        kill(pids[i], SIGTERM);
        waitpid(pids[i], NULL, 0);
    }
    free(pids);

    // Довычитываем оставшиеся в очереди сообщения перед выходом
    while (receive_observation(mq, &obs) == 0) {
        printf("[Main] Фінальний запис від %s (%s): %d зубр(ів)\n",
            obs.observing_personname, obs.territory_name, obs.bison_number);

        fprintf(csv, "%s,%s,%s,%d,%.*s\n",
            obs.observation_time,
            obs.territory_name,
            obs.observing_personname,
            obs.bison_number,
            (int)sizeof(obs.bison_age),
            obs.bison_age);

        fflush(csv);
    }

    fclose(csv);
    cleanup_queue(mq);

    printf("[Main] Работа завершена. Данные в results/observations.csv\n");
    return 0;
}
