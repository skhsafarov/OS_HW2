#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <time.h>
#include <string.h>

typedef struct {
    double inheritance;
    double share;
    double received;
    int index;
    sem_t *semaphore;
} Heir;

// Функция для проверки доли каждого наследника
void check_heir(Heir *heir) {
    // Генерация случайного времени ожидания
    srand(time(NULL) ^ getpid());
    usleep(rand() % 100000);

    // Получение доступа к семафору
    if (sem_wait(heir->semaphore) < 0) {
        perror("sem_wait");
        exit(1);
    }

    // Вычисление ожидаемой доли и вывод информации о наследнике
    double expected = heir->inheritance * heir->share;
    printf("Наследник %d: ожидаемая доля %f, полученная доля %f\n", heir->index, expected, heir->received);

    // Проверка на соответствие ожидаемой и полученной доли
    if (expected != heir->received) {
        printf("Адвокат нечестен! Наследник %d ожидал: %f, получил: %f\n", heir->index, expected, heir->received);
    }
    printf("\n");

    // Освобождение семафора
    if (sem_post(heir->semaphore) < 0) {
        perror("sem_post");
        exit(1);
    }
}

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
        heirs[i].index = i + 1;
    }

    // Инициализация семафора
    sem_t semaphore;
    if (sem_init(&semaphore, 0, 1) < 0) {
        perror("sem_init");
        exit(1);
    }

    // Создание процессов для каждого наследника и вызов функции проверки
    for (int i = 0; i < NUM_HEIRS; i++) {
        heirs[i].semaphore = &semaphore;
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            exit(1);
        } else if (pid == 0) {
            check_heir(&heirs[i]);
            exit(0);
        }
    }

    // Ожидание завершения всех процессов-наследников
    for (int i = 0; i < NUM_HEIRS; i++) {
        int status;
        if (waitpid(-1, &status, 0) < 0) {
            perror("waitpid");
            exit(1);
        }
    }

    // Уничтожение семафора
    if (sem_destroy(&semaphore) < 0) {
        perror("sem_destroy");
        exit(1);
    }

    return 0;
}
