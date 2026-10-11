#include <stdio.h> 
#include <stdlib.h>  
#include <string.h>
#include <unistd.h> 
#include <signal.h> 
#include <time.h>   
#include <sys/wait.h>  
#include "common.h"   

int main(int argc, char* argv[]) {

    // argv[0] = имя бинарника, argv[1] = N (число экспедиций), argv[2] = T (время сессии)
    if (argc < 3) {
        printf("Использование: %s <N_экспедиций> <T_время_сессии_сек>\n", argv[0]);
        return 1;
    }

    int n_expeditions = atoi(argv[1]);
    int session_time = atoi(argv[2]);

    printf("[Main] Запуск системы: %d экспедиций, время работы %d сек\n", n_expeditions, session_time);

    mqd_t mq = ipc_server_init();  // участник 3 создает новую очередь в ядре Linux на чтение

    FILE* csv = fopen("results/observations.csv", "w");
    if (!csv) {
        perror("Ошибка открытия results/observations.csv");
        ipc_unlink();
        return 1;
    }

    fprintf(csv, "Observer,Territory,Count,Age,Timestamp\n");
    fflush(csv);

    pid_t* pids = malloc(sizeof(pid_t) * n_expeditions);

    for (int i = 0; i < n_expeditions; i++) {
        int pipefd[2]; // pipefd[0] чтение | pipefd[1] запись

        if (pipe(pipefd) == -1) {
            perror("pipe error");
            exit(1);
        }
        pid_t pid = fork();

		if (pid == 0) { // дочерний процесс
            
            // Ребенок не будет писать в pipe, поэтому закрываем дескриптор записи
            close(pipefd[1]);

            char name_buf[64];
            snprintf(name_buf, sizeof(name_buf), "Наблюдатель_%d", i + 1);

            run_expedition(pipefd[0], name_buf);  // участник 2. будем надеяться он закрыл читателя...

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
            exit(1);
        }
    }

    time_t start_time = time(NULL);
    ObservationData obs;

	while (time(NULL) - start_time < session_time) {  // работаем session_time секунд

        if (ipc_receive_observation(mq, &obs) == 0) {

            printf("[Main] Запись от %s (%s): %d зубра(ов)\n",
                obs.observing_personname, obs.territory_name, obs.bison_number);

            fprintf(csv, "%s,%s,%d,%s,%s\n",
                obs.observing_personname,
                obs.territory_name,
                obs.bison_number,
                obs.bison_age,
                obs.observation_time);

            fflush(csv);
        }
        else
            usleep(100000); // 100мс ждем
    }

    printf("[Main] Время сессии истекло. Остановка процессов...\n");

    for (int i = 0; i < n_expeditions; i++) {
        kill(pids[i], SIGTERM);
        waitpid(pids[i], NULL, 0);
    }
    free(pids);

    fclose(csv);
    ipc_close(mq);  // творение участника 3 будет закрыто
    ipc_unlink();  // и хлам тоже удаляем

    printf("[Main] Работа завершена. Данные в results/observations.csv\n");
    return 0;
}