#include <fcntl.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <time.h>

const char *sem_name = "/semaphore";

typedef struct {
    double inheritance;
    double share;
    double received;
    int index;
} Heir;

void check_heir(int pipefd[2]) {
    close(pipefd[1]); // Закрываем дескриптор записи, так как он не используется


    Heir received_heir;
    read(pipefd[0], &received_heir, sizeof(Heir));

    // Устанавливаем зерно для генерации случайных чисел
    srand(time(NULL) ^ getpid());
    // Создаем случайную задержку
    usleep(rand() % 100000);

    printf("Наследник %d: ожидаемая сумма %lf, полученная сумма %lf\n", received_heir.index, received_heir.inheritance, received_heir.received);
    if (received_heir.inheritance != received_heir.received) {
        printf("Адвокат нечестен\n");
    }
    printf("\n");
    close(pipefd[0]);
    exit(0);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Использование: %s <имя_файла>\n", argv[0]);
        exit(1);
    }

    FILE *file = fopen(argv[1], "r");
    if (file == NULL) {
        perror("fopen");
        exit(1);
    }

    double inheritance;
    int NUM_HEIRS = 8;
    Heir heirs[NUM_HEIRS];

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "inheritance;", 12) == 0) {
            sscanf(line, "inheritance; %lf", &inheritance);
        } else if (strncmp(line, "shares;", 7) == 0) {
            sscanf(line, "shares; %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf",
                   &heirs[0].share, &heirs[1].share,
                   &heirs[2].share, &heirs[3].share,
                   &heirs[4].share, &heirs[5].share,
                   &heirs[6].share, &heirs[7].share);
        } else if (strncmp(line, "received;", 9) == 0) {
            sscanf(line, "received; %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf",
                   &heirs[0].received, &heirs[1].received,
                   &heirs[2].received, &heirs[3].received,
                   &heirs[4].received, &heirs[5].received,
                   &heirs[6].received, &heirs[7].received);
        }
    }

    fclose(file);

    // Создаем именованный семафор
    sem_t *sem = sem_open(sem_name, O_CREAT | O_EXCL, 0666, 1);
    if (sem == SEM_FAILED) {
        perror("sem_open");
        exit(1);
    }

    // Создаем канал (pipe)
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(1);
    }

    for (int i = 0; i < NUM_HEIRS; i++) {
        heirs[i].inheritance = inheritance * heirs[i].share;
        heirs[i].index = i + 1;
    }

    for (int i = 0; i < NUM_HEIRS; i++) {
        if (pipe(pipefd) == -1) {
            perror("pipe");
            exit(1);
        }

        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            exit(1);
        }

        if (pid == 0) {
            // Дочерний процесс
            check_heir(pipefd);
        } else {
            // Родительский процесс
            sem_wait(sem); // ждем разрешения на доступ

            write(pipefd[1], &heirs[i], sizeof(Heir));

            sem_post(sem); // освобождаем семафор
            close(pipefd[1]); // закрываем дескриптор записи
        }
    }

    for (int i = 0; i < NUM_HEIRS; i++) {
        wait(NULL); // ждем завершения всех дочерних процессов
    }

    sem_close(sem);
    sem_unlink(sem_name);

    return 0;
}