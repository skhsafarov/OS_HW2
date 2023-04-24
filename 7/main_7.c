#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <string.h>

typedef struct {
    double inheritance;
    double share;
    double received;
} Heir;

int main(int argc, char *argv[]) {
    // Проверка на наличие аргумента с именем файла
    if (argc != 2) {
        printf("Usage: %s input_file\n", argv[0]);
        exit(1);
    }

    char *input_file_name = argv[1];

    int NUM_HEIRS = 8;
    double inheritance;
    Heir heirs[NUM_HEIRS];
    double shares[NUM_HEIRS];
    double received[NUM_HEIRS];
    char SEM_NAME[] = "/heir_semaphore";
    int SHM_KEY = 1234;

    // Чтение данных из файла
    FILE *input_file = fopen(input_file_name, "r");
    if (input_file == NULL) {
        perror("fopen");
        exit(1);
    }

    char line[256];
    char *token;

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

    if (fclose(input_file) < 0) {
        perror("fclose");
        exit(1);
    }

    // Инициализация структуры наследников
    for (int i = 0; i < NUM_HEIRS; i++) {
        heirs[i].inheritance = inheritance;
        heirs[i].share = shares[i];
        heirs[i].received = received[i];
    }

    // Создание и открытие семафора
    sem_t *sem = sem_open(SEM_NAME, O_CREAT, 0644, 1);
    if (sem == SEM_FAILED) {
        perror("sem_open");
        exit(1);
    }

    // Создание разделяемой памяти
    int shmid = shmget(SHM_KEY, sizeof(heirs), IPC_CREAT | 0666);
    if (shmid < 0) {
        perror("shmget");
        exit(1);
    }

    Heir *shm_heirs = (Heir *) shmat(shmid, NULL, 0);
    if (shm_heirs == (Heir *) -1) {
        perror("shmat");
        exit(1);
    }

    for (int i = 0; i < NUM_HEIRS; i++) {
        shm_heirs[i] = heirs[i];
    }

    printf("Press Enter to exit...\n");
    getchar();

    // Отключение разделяемой памяти от адресного пространства процесса
    shmdt(shm_heirs);
    // Удаление разделяемой памяти
    shmctl(shmid, IPC_RMID, NULL);
    // Закрытие семафора
    sem_close(sem);
    // Удаление семафора из системы
    sem_unlink(SEM_NAME);

    return 0;
}
