#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>

typedef struct {
    double inheritance;
    double share;
    double received;
    int index;
} Heir;

void lock_semaphore(int semid) {
    struct sembuf op;
    op.sem_num = 0;
    op.sem_op = -1;
    op.sem_flg = 0;
    if (semop(semid, &op, 1) < 0) {
        perror("semop lock");
        exit(-1);
    }
}

void unlock_semaphore(int semid) {
    struct sembuf op;
    op.sem_num = 0;
    op.sem_op = 1;
    op.sem_flg = 0;
    if (semop(semid, &op, 1) < 0) {
        perror("semop unlock");
        exit(-1);
    }
}

int main(int argc, char *argv[]) {
    int NUM_HEIRS = 8;
    int SHM_KEY = 1234;
    int SEM_KEY = 5678;
    unsigned long SHM_SIZE = (sizeof(Heir) * NUM_HEIRS);

    if (argc != 2) {
        printf("Usage: %s heir_index\n", argv[0]);
        exit(1);
    }

    int index = atoi(argv[1]);
    if (index < 1 || index > NUM_HEIRS) {
        printf("Invalid heir number. It should be between 1 and %d\n", NUM_HEIRS);
        exit(1);
    }

    // Получение идентификатора разделяемой памяти
    int shmid = shmget(SHM_KEY, SHM_SIZE, 0666);
    if (shmid == -1) {
        perror("shmget");
        exit(1);
    }

    // Подключение разделяемой памяти к адресному пространству процесса
    Heir *shared_memory = shmat(shmid, NULL, 0);
    if (shared_memory == (Heir *)-1) {
        perror("shmat");
        exit(1);
    }

    // Получение идентификатора семафора
    int sem_id = semget(SEM_KEY, 1, 0666);
    if (sem_id < 0) {
        perror("semget");
        exit(1);
    }

    lock_semaphore(sem_id);

    // Получение указателя на данные текущего наследника
    Heir *heir = &shared_memory[index - 1];
    double expected = heir->inheritance * heir->share;
    printf("Наследник %d: ожидаемая доля %f, полученная доля %f\n", heir->index, expected, heir->received);
    if (expected != heir->received) {
        printf("Адвокат нечестен! Наследник %d ожидал: %f, получил: %f\n", heir->index, expected, heir->received);
    }

    unlock_semaphore(sem_id);

    // Отключение разделяемой памяти от адресного пространства процесса
    shmdt(shared_memory);

    return 0;
}