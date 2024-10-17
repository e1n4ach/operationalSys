#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/cdefs.h>
#include <fcntl.h>
#include <stdbool.h>
#include <string.h>
#include <sys/wait.h>


#define BUFFER_SIZE 256


//function for check is it prime
bool is_prime(int num){
    if (num <= 1) {
        return false;
    }
    for (int i = 2; i * i <= num; i++){
        if (num % i == 0){
            return false;
        }
    }
    return true;
}

int main(){
    int pipefd[2];
    pid_t pid;
    int number;
    char buffer[BUFFER_SIZE];

    if (pipe(pipefd) == -1){
        return 1; //if error with creating pipe
    }

    pid = fork();

    if (pid < 0){
        return 2; //if error with creating child
    }

    if (pid > 0) { //main process
        close(pipefd[0]); //we dont need to read in head process

        printf("Введите число в виде <число <X>>:");

        if (fgets(buffer, BUFFER_SIZE, stdin) == NULL){  // if buffer is empty
            printf("Ошибка ввода");
            return 3;
        }
            
        if (sscanf(buffer, "число %d", &number) != 1) {   // take a number from input
            printf("Неверный формат, ввидите <число X> \n");
            return 3;
        }

        if (number < 0) {    // if number < 0, need to exit from process
            write(pipefd[1], &number, sizeof(int));
            close(pipefd[1]);
            exit(EXIT_SUCCESS);
        }

        if (write(pipefd[1], &number, sizeof(int)) == -1){  // send the number to the child by pipe
            printf("Error with writting to the pipe");
            return 4;
        }

        wait(NULL);
    } else{ //child process
        close(pipefd[1]);

        if (read(pipefd[0], &number, sizeof(int)) == -1) {
            printf("Error with writting to the pipe");
            return 5;
        }

        if (number < 0) {
            printf("Полученно отрицательное число. Процессы завершены.\n");
            exit(EXIT_SUCCESS);
        }

        if (is_prime(number)){
            printf("Число %d простое.  Процессы завершены.\n", number);
            exit(EXIT_SUCCESS);
        } else{
            printf(" Число %d составное и записано в файл \n", number);

            FILE *file = fopen("compose_numbers.txt", "a");
            if (file == NULL) {
                printf("Error with oppening file");
                return 6;
            }
            fprintf(file, "%d\n", number);
            fclose(file);
        }
        close(pipefd[0]);
    }
    return 0;
}

    
