#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <semaphore.h>
#include <time.h>
#include <string.h>

// Определение структуры Heir для хранения информации о каждом наследнике
typedef struct {
    double inheritance;
    double share;
    double received;
    int index;
} Heir;

sem_t *semaphore;

// Функция проверки наследства для каждого наследника
void check_heir(Heir *heir) {
    // Устанавливаем зерно для генерации случайных чисел
    srand(time(NULL) ^ getpid());
    // Создаем случайную задержку
    usleep(rand() % 100000);
    // Захватываем семафор для синхронизации доступа к данным
    if (sem_wait(semaphore) < 0) {
        perror("sem_wait");
        exit(1);
    }
    double expected = heir->inheritance * heir->share;
    printf("Наследник %d: ожидаемая доля %f, полученная доля %f\n", heir->index, expected, heir->received);
    if (expected != heir->received) {
        printf("Адвокат нечестен! Наследник %d ожидал: %f, получил: %f\n", heir->index, expected, heir->received);
    }
    printf("\n");
    // Освобождаем семафор
    if (sem_post(semaphore) < 0) {
        perror("sem_post");
        exit(1);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("You must use input file");
        exit(1);
    }

    char *input_file_name = argv[1];

    int NUM_HEIRS = 8;
    char SEMAPHORE_NAME[] = "/heirs_semaphore";
    double inheritance;
    Heir heirs[NUM_HEIRS];
    double shares[NUM_HEIRS];
    double received[NUM_HEIRS];

    // Открываем файл с входными данными
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

    // Закрываем файл с входными данными
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

    // Создаем и инициализируем семафор для синхронизации доступа к данным
    semaphore = sem_open(SEMAPHORE_NAME, O_CREAT, 0666, 1);
    if (semaphore == SEM_FAILED) {
        perror("sem_open");
        exit(1);
    }

    pid_t pids[NUM_HEIRS];

    // Создаем процессы-наследников, каждый из которых будет проверять свою долю наследства
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

    // Ожидаем завершение всех процессов-наследников
    for (int i = 0; i < NUM_HEIRS; i++) {
        int status;
        if (waitpid(pids[i], &status, 0) < 0) {
            perror("waitpid");
            exit(1);
        }
    }

    // Закрываем и удаляем семафор
    if (sem_close(semaphore) < 0) {
        perror("sem_close");
        exit(1);
    }

    if (sem_unlink(SEMAPHORE_NAME) < 0) {
        perror("sem_unlink");
        exit(1);
    }

    return 0;
}