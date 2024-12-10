CC = gcc
CFLAGS = -Wall -Wextra -pedantic -Werror -O2
ZMQFLAGS = `pkg-config --cflags --libs libzmq`

all: controller worker

controller: controller.c
	$(CC) $(CFLAGS) controller.c -o controller $(ZMQFLAGS)

worker: worker.c
	$(CC) $(CFLAGS) worker.c -o worker $(ZMQFLAGS)

clean:
	rm -f controller worker
