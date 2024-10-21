TARGET = solution

CC = gcc

CFLAGS = -pthread -Wall -Wextra -std=c99

SRC = solution.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)

rebuild: clean all