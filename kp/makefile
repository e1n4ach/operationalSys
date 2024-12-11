CC = gcc
CFLAGS = -Wall -Wextra -O2 -pthread -g -std=c11
LDFLAGS = -lyaml -pthread -latomic

SRC = main.c config.c graph_utils.c executor.c
OBJ = $(SRC:.c=.o)
EXEC = dag_executor

all: $(EXEC)

$(EXEC): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(EXEC)
