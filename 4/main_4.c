#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <time.h>
#include <string.h>

// Определение структуры Heir для хранения информации о каждом наследнике
typedef struct {
    double inheritance;
    double share;
    double received;
    int index;
} Heir;

int sem_id;

void lock_semaphore(int semid) {
    struct sembuf op;
    op.sem_num = 0;
    op.sem_op = -1;
    op.sem_flg = 0;
    if (semop(semid, &op, 1) < 0) {
        perror("semop lock");
        exit(1);
    }
}

void unlock_semaphore(int semid) {
    struct sembuf op;
    op.sem_num = 0;
    op.sem_op = 1;
    op.sem_flg = 0;
    if (semop(semid, &op, 1) < 0) {
        perror("semop unlock");
        exit(1);
    }
}

// Функция проверки наследства для каждого наследника
void check_heir(Heir *heir) {
    // Устанавливаем зерно для генерации случайных чисел
    srand(time(NULL) ^ getpid());
    // Создаем случайную задержку
    usleep(rand() % 100000);
    lock_semaphore(sem_id);
    double expected = heir->inheritance * heir->share;
    printf("Наследник %d: ожидаемая доля %f, полученная доля %f\n", heir->index, expected, heir->received);
    if (expected != heir->received) {
        printf("Адвокат нечестен! Наследник %d ожидал: %f, получил: %f\n", heir->index, expected, heir->received);
    }
    printf("\n");
    unlock_semaphore(sem_id);
}

int main(int argc, char *argv[]) {
    // Проверка аргументов командной строки и получение имени файла с входными данными
    if (argc != 2) {
        printf("You must use input file");
        exit(1);
    }

    char *input_file_name = argv[1];

    int NUM_HEIRS = 8;
    double inheritance;
    double shares[NUM_HEIRS];
    double received[NUM_HEIRS];
    Heir heirs[NUM_HEIRS];

    // Открытие файла с входными данными
    FILE *input_file = fopen(input_file_name, "r");
    if (input_file == NULL) {
        perror("fopen");
        exit(1);
    }

    char line[256];
    char *token;

    // Чтение данных о наследстве, долях и полученных суммах из файла
    fgets(line, sizeof(line), input_file);
    sscanf(line, "inheritance; %lf", &inheritance);

    fgets(line, sizeof(line), input_file);
    token = strtok(line, "shares; ,\n");
    for (int i = 0; token != NULL && i < NUM_HEIRS; i++) {
        shares[i] = atof(token);
        token = strtok(NULL, " ,\n");
    }

    fgets(line, sizeof(line), input_file);
    token = strtok(line, "received; ,\n");
    for (int i = 0; token != NULL && i < NUM_HEIRS; i++) {
        received[i] = atof(token);
        token = strtok(NULL, " ,\n");
    }

    // Закрытие файла с входными данными
    if (fclose(input_file) < 0) {
        perror("fclose");
        exit(1);
    }

    // Заполняем массив структур наследников данными
    for (int i = 0; i < NUM_HEIRS; i++) {
        heirs[i].inheritance = inheritance;
        heirs[i].share = shares[i];
        heirs[i].received = received[i];
        heirs[i].index = i + 1;
    }

    // Создание ключа и семафора для синхронизации доступа к данным наследников
    key_t key = ftok("heirs", 65);
    sem_id = semget(key, 1, 0666 | IPC_CREAT);
    if (sem_id < 0) {
        perror("semget");
        exit(1);
    }

    if (semctl(sem_id, 0, SETVAL, 1) < 0) {
        perror("semctl");
        exit(1);
    }

    // Создание процессов для каждого наследника
    pid_t pids[NUM_HEIRS];

    for (int i = 0; i < NUM_HEIRS; i++) {
        pids[i] = fork();
        if (pids[i] < 0) {
            perror("fork");
            exit(1);
        } else if (pids[i] == 0) {
            check_heir(&heirs[i]);
            exit(0);
        }
    }

    // Ожидание завершения всех процессов наследников
    for (int i = 0; i < NUM_HEIRS; i++) {
        int status;
        if (waitpid(pids[i], &status, 0) < 0) {
            perror("waitpid");
            exit(1);
        }
    }

    // Удаление семафора
    if (semctl(sem_id, 0, IPC_RMID) < 0) {
        perror("semctl remove");
        exit(1);
    }

    return 0;
}
