#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zmq.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: worker <endpoint>\n");
        return 1;
    }

    const char *endpoint = argv[1];

    void *context = zmq_ctx_new();
    void *socket = zmq_socket(context, ZMQ_REP);
    if (zmq_bind(socket, endpoint) != 0) {
        fprintf(stderr, "Error: cannot bind to %s\n", endpoint);
        return 1;
    }

    long long total_time_ms = 0;
    int is_running = 0;
    struct timeval start_tv;

    while (1) {
        zmq_msg_t msg;
        zmq_msg_init(&msg);
        int rc = zmq_msg_recv(&msg, socket, 0);
        if (rc == -1) {
            zmq_msg_close(&msg);
            break;
        }

        char *req_str = malloc(rc+1);
        memcpy(req_str, zmq_msg_data(&msg), rc);
        req_str[rc] = '\0';
        zmq_msg_close(&msg);

        char *cmd = strtok(req_str, " ");
        if (!cmd) {
            zmq_send(socket, "Error: Invalid command", 22, 0);
            free(req_str);
            continue;
        }

        if (strcmp(cmd, "ping") == 0) {
            zmq_send(socket, "Ok", 2, 0);

        } else if (strcmp(cmd, "exec") == 0) {
            char *subcmd = strtok(NULL, " ");
            if (!subcmd) {
                zmq_send(socket, "Error: Missing subcommand", 24, 0);
                free(req_str);
                continue;
            }

            if (strcmp(subcmd, "start") == 0) {
                if (!is_running) {
                    gettimeofday(&start_tv, NULL);
                    is_running = 1;
                }
                const char *ptr = strrchr(endpoint, '_');
                int node_id = -1;
                if (ptr) {
                    node_id = atoi(ptr+1);
                }
                char resp[64];
                snprintf(resp, sizeof(resp), "Ok:%d", node_id);
                zmq_send(socket, resp, strlen(resp), 0);

            } else if (strcmp(subcmd, "stop") == 0) {
                const char *ptr = strrchr(endpoint, '_');
                int node_id = -1;
                if (ptr) {
                    node_id = atoi(ptr+1);
                }
                char resp[64];

                if (is_running) {
                    struct timeval now;
                    gettimeofday(&now, NULL);
                    long long elapsed = (now.tv_sec - start_tv.tv_sec)*1000LL + (now.tv_usec - start_tv.tv_usec)/1000LL;
                    total_time_ms += elapsed;
                    is_running = 0;
                }
                snprintf(resp, sizeof(resp), "Ok:%d", node_id);
                zmq_send(socket, resp, strlen(resp), 0);

            } else if (strcmp(subcmd, "time") == 0) {
                const char *ptr = strrchr(endpoint, '_');
                int node_id = -1;
                if (ptr) {
                    node_id = atoi(ptr+1);
                }
                char resp[64];

                long long current_time = total_time_ms;
                if (is_running) {
                    struct timeval now;
                    gettimeofday(&now, NULL);
                    long long elapsed = (now.tv_sec - start_tv.tv_sec)*1000LL + (now.tv_usec - start_tv.tv_usec)/1000LL;
                    current_time += elapsed;
                }
                snprintf(resp, sizeof(resp), "Ok:%d: %lld", node_id, current_time);
                zmq_send(socket, resp, strlen(resp), 0);

            } else {
                const char *ptr = strrchr(endpoint, '_');
                int node_id = -1;
                if (ptr) {
                    node_id = atoi(ptr+1);
                }
                char resp[128];
                snprintf(resp, sizeof(resp), "Error:%d: Unknown subcommand", node_id);
                zmq_send(socket, resp, strlen(resp), 0);
            }

        } else {
            zmq_send(socket, "Error: Unknown command", 22, 0);
        }

        free(req_str);
    }

    zmq_close(socket);
    zmq_ctx_destroy(context);
    return 0;
}
