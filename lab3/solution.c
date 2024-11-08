#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdbool.h>
#include <string.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/stat.h>

#define FILE_PATH "shared_memory_file"
#define BUFFER_SIZE 256

bool is_prime(int num) {
    if (num <= 1) {
        return false;
    }
    for (int i = 2; i * i <= num; i++) {
        if (num % i == 0) {
            return false;
        }
    }
    return true;
}

// status
volatile sig_atomic_t exit_flag = 0;

// signal for end
void handle_exit() {
    exit_flag = 1;
}

int main() {
    int number;
    pid_t pid;

    // create file
    int fd = open(FILE_PATH, O_RDWR | O_CREAT | O_TRUNC, 0666);
    if (fd == -1) {
        perror("Ошибка при открытии файла");
        exit(EXIT_FAILURE);
    }

    // size of file = int
    if (ftruncate(fd, sizeof(int)) == -1) {
        perror("Ошибка при установке размера файла");
        close(fd);
        exit(EXIT_FAILURE);
    }

    // show file to memory
    int *shared_mem = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (shared_mem == MAP_FAILED) {
        perror("Ошибка отображения файла");
        close(fd);
        exit(EXIT_FAILURE);
    }

    close(fd);

    pid = fork();
    if (pid < 0) {
        perror("Ошибка при создании процесса");
        munmap(shared_mem, sizeof(int));
        exit(EXIT_FAILURE);
    }

    if (pid > 0) { 
        struct sigaction sa;
        sa.sa_handler = &handle_exit;
        sigaction(SIGUSR1, &sa, NULL);

        printf("Введите число в формате <число X>: ");
        char buffer[BUFFER_SIZE];
        
        if (fgets(buffer, BUFFER_SIZE, stdin) == NULL) {
            printf("Ошибка ввода\n");
            exit(EXIT_FAILURE);
        }

        if (sscanf(buffer, "число %d", &number) != 1) {
            printf("Неверный формат, введите <число X>\n");
            exit(EXIT_FAILURE);
        }

        if (number < 0) {
            *shared_mem = number;  
            kill(pid, SIGUSR1);    // cignall for child process
            wait(NULL);
            munmap(shared_mem, sizeof(int));
            unlink(FILE_PATH);
            exit(EXIT_SUCCESS);
        }

        // write number
        *shared_mem = number;
        kill(pid, SIGUSR1); // signal for child process

        // wait end of child process
        wait(NULL);
        munmap(shared_mem, sizeof(int));
        unlink(FILE_PATH);

    } else {
        struct sigaction sa;
        sa.sa_handler = &handle_exit;
        sigaction(SIGUSR1, &sa, NULL);

        while (!exit_flag) {
            pause();
        }

        if (*shared_mem < 0) {
            printf("Получено отрицательное число. Процессы завершены.\n");
            exit(EXIT_SUCCESS);
        }

        if (is_prime(*shared_mem)) {
            printf("Число %d простое. Процессы завершены.\n", *shared_mem);
            exit(EXIT_SUCCESS);
        } else {
            printf("Число %d составное и записано в файл.\n", *shared_mem);
            FILE *file = fopen("compose_numbers.txt", "a");
            if (file == NULL) {
                perror("Ошибка при открытии файла");
                exit(EXIT_FAILURE);
            }
            fprintf(file, "%d\n", *shared_mem);
            fclose(file);
        }
        exit(EXIT_SUCCESS);
    }
}