#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include "string.h"

typedef struct {
    double inheritance;
    double share;
    double received;
    int index;
} Heir;

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s input_file\n", argv[0]);
        exit(1);
    }

    int NUM_HEIRS = 8;
    double inheritance;
    double shares[NUM_HEIRS];
    double received[NUM_HEIRS];


    // Чтение данных из файла
    char *input_file_name = argv[1];
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


    // Создание разделяемой памяти для хранения данных
    int SHM_KEY = 1234;
    int SEM_KEY = 5678;
    unsigned long SHM_SIZE = (sizeof(Heir) * NUM_HEIRS);
    int shmid = shmget(SHM_KEY, SHM_SIZE, 0666 | IPC_CREAT);
    if (shmid == -1) {
        perror("shmget");
        exit(1);
    }

    Heir *shared_memory = shmat(shmid, NULL, 0);
    if (shared_memory == (Heir *) -1) {
        perror("shmat");
        exit(1);
    }

    // Запись данных в разделяемую память
    for (int i = 0; i < NUM_HEIRS; i++) {
        shared_memory[i].inheritance = inheritance;
        shared_memory[i].share = shares[i];
        shared_memory[i].received = received[i];
        shared_memory[i].index = i + 1;
    }

    // Создание семафора для синхронизации доступа к разделяемой памяти
    int sem_id = semget(SEM_KEY, 1, 0666 | IPC_CREAT);
    if (sem_id == -1) {
        perror("semget");
        exit(1);
    }

    if (semctl(sem_id, 0, SETVAL, 1) == -1) {
        perror("semctl");
        exit(1);
    }

    printf("Press Enter to exit...\n");
    getchar();

    shmdt(shared_memory);

    shmctl(shmid, IPC_RMID, NULL);
    semctl(sem_id, 0, IPC_RMID);

    return 0;
}