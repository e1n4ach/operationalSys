TARGET = solution

CC = gcc
CFLAGS = -Wall -Wextra -pedantic -Werror -g

SRC = solution.c
OBJ = $(SRC:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(OBJ) $(TARGET) compose_numbers.txt shared_memory_file