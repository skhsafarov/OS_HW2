#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <sys/shm.h>
#include <sys/ipc.h>

typedef struct {
    double inheritance;
    double share;
    double received;
} Heir;

int main(int argc, char *argv[]) {
    int NUM_HEIRS = 8;
    char SEM_NAME[] = "/heir_semaphore";
    int SHM_KEY = 1234;
    if (argc != 2) {
        printf("Usage: %s <heir_number>\n", argv[0]);
        exit(1);
    }

    int index = atoi(argv[1]);
    if (index < 1 || index > NUM_HEIRS) {
        printf("Invalid heir number. It should be between 1 and %d\n", NUM_HEIRS);
        exit(1);
    }

    // Открытие семафора
    sem_t *sem = sem_open(SEM_NAME, 0);
    if (sem == SEM_FAILED) {
        perror("sem_open");
        exit(1);
    }

    // Получение идентификатора разделяемой памяти
    int shmid = shmget(SHM_KEY, sizeof(Heir) * NUM_HEIRS, 0666);
    if (shmid < 0) {
        perror("shmget");
        exit(1);
    }

    // Получение адреса разделяемой памяти
    Heir *shm_heirs = (Heir *)shmat(shmid, NULL, 0);
    if (shm_heirs == (Heir *)-1) {
        perror("shmat");
        exit(1);
    }

    // Ожидание доступа к разделяемой памяти
    sem_wait(sem);
    double expected = shm_heirs[index - 1].inheritance * shm_heirs[index - 1].share;
    printf("Наследник %d: ожидаемая доля %f, полученная доля %f\n", index, expected, shm_heirs[index - 1].received);
    if (expected != shm_heirs[index - 1].received) {
        printf("Адвокат нечестен! Наследник %d ожидал: %f, получил: %f\n", index, expected, shm_heirs[index - 1].received);
    }
    printf("\n");
    // Освобождение доступа к разделяемой памяти
    sem_post(sem);

    // Отключение разделяемой памяти от адресного пространства процесса
    shmdt(shm_heirs);
    // Закрытие семафора
    sem_close(sem);

    return 0;
}
