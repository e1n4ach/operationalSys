#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <zmq.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

typedef struct Node {
    int id;
    pid_t pid;
    char endpoint[256];
    struct Node *left;
    struct Node *right;
    int available; 
} Node;

Node *root = NULL;

Node* insert_node(Node *root, int id, pid_t pid, const char *endpoint) {
    if (root == NULL) {
        Node *new_node = (Node*)malloc(sizeof(Node));
        new_node->id = id;
        new_node->pid = pid;
        new_node->available = 1;
        strncpy(new_node->endpoint, endpoint, sizeof(new_node->endpoint));
        new_node->endpoint[sizeof(new_node->endpoint)-1] = '\0';
        new_node->left = NULL;
        new_node->right = NULL;
        return new_node;
    }
    if (id < root->id) {
        root->left = insert_node(root->left, id, pid, endpoint);
    } else if (id > root->id) {
        root->right = insert_node(root->right, id, pid, endpoint);
    }
    return root;
}

Node* find_node(Node *root, int id) {
    if (!root) return NULL;
    if (id == root->id) return root;
    if (id < root->id) return find_node(root->left, id);
    return find_node(root->right, id);
}

void collect_nodes(Node *root, Node ***arr, int *count) {
    if (!root) return;
    collect_nodes(root->left, arr, count);
    (*arr)[*count] = root;
    (*count)++;
    collect_nodes(root->right, arr, count);
}

int send_request(void *context, const char *endpoint, const char *request, char *reply, size_t reply_size) {
    void *socket = zmq_socket(context, ZMQ_REQ);
    if (!socket) return -1;

    int timeout = 1000; 
    zmq_setsockopt(socket, ZMQ_RCVTIMEO, &timeout, sizeof(int));

    if (zmq_connect(socket, endpoint) != 0) {
        zmq_close(socket);
        return -1;
    }

    zmq_msg_t msg;
    zmq_msg_init_size(&msg, strlen(request));
    memcpy(zmq_msg_data(&msg), request, strlen(request));
    if (zmq_msg_send(&msg, socket, 0) == -1) {
        zmq_msg_close(&msg);
        zmq_close(socket);
        return -1;
    }
    zmq_msg_close(&msg);

    zmq_msg_t resp;
    zmq_msg_init(&resp);
    int rc = zmq_msg_recv(&resp, socket, 0);
    if (rc == -1) {
        zmq_msg_close(&resp);
        zmq_close(socket);
        return -1;
    }
    int len = rc;
    if (len < (int)reply_size) {
        memcpy(reply, zmq_msg_data(&resp), len);
        reply[len] = '\0';
    } else {
        memcpy(reply, zmq_msg_data(&resp), reply_size-1);
        reply[reply_size-1] = '\0';
    }

    zmq_msg_close(&resp);
    zmq_close(socket);
    return 0;
}

int main() {
    void *context = zmq_ctx_new();
    if (!context) {
        fprintf(stderr, "Error: cannot create ZMQ context\n");
        return 1;
    }

    char line[1024];
    while (1) {
        printf("> ");
        fflush(stdout);
        if (!fgets(line, sizeof(line), stdin)) break;
        char *cmd = strtok(line, " \t\n");
        if (!cmd) continue;

        if (strcmp(cmd, "create") == 0) {
            char *id_str = strtok(NULL, " \t\n");
            if (!id_str) {
                printf("Error: Invalid command\n");
                continue;
            }
            int node_id = atoi(id_str);

            Node *found = find_node(root, node_id);
            if (found) {
                printf("Error: Already exists\n");
                continue;
            }

            char endpoint[256];
            snprintf(endpoint, sizeof(endpoint), "ipc:///tmp/node_%d", node_id);

            pid_t pid = fork();
            if (pid == -1) {
                perror("fork");
                printf("Error: Cannot create node\n");
                continue;
            }
            if (pid == 0) {
                execl("./worker", "./worker", endpoint, (char*)NULL);
                perror("exec");
                exit(1);
            }

            root = insert_node(root, node_id, pid, endpoint);
            printf("Ok: %d\n", pid);

        } else if (strcmp(cmd, "exec") == 0) {
            char *id_str = strtok(NULL, " \t\n");
            if (!id_str) {
                printf("Error: Invalid command\n");
                continue;
            }
            int node_id = atoi(id_str);
            Node *node = find_node(root, node_id);
            if (!node) {
                printf("Error:%d: Not found\n", node_id);
                continue;
            }

            char *subcmd = strtok(NULL, " \t\n");
            if (!subcmd) {
                printf("Error:%d: Invalid subcommand\n", node_id);
                continue;
            }

            if (strcmp(subcmd, "start") != 0 && strcmp(subcmd, "stop") != 0 && strcmp(subcmd, "time") != 0) {
                printf("Error:%d: Unknown subcommand\n", node_id);
                continue;
            }

            char request[256];
            snprintf(request, sizeof(request), "exec %s", subcmd);

            char reply[256];
            if (send_request(context, node->endpoint, request, reply, sizeof(reply)) != 0) {
                printf("Error:%d: Node is unavailable\n", node_id);
                node->available = 0;
                continue;
            }

            if (strncmp(reply, "Ok", 2) == 0) {
                printf("%s\n", reply); 
            } else if (strncmp(reply, "Error", 5) == 0) {
                printf("%s\n", reply);
            } else {
                printf("Error:%d: Unexpected response\n", node_id);
            }

        } else if (strcmp(cmd, "pingall") == 0) {
            {
                int node_count = 0;
                {
                    struct {
                        Node* stack[1024];
                        int top;
                    } st;
                    st.top = -1;
                    Node *cur = root;
                    while (cur || st.top >= 0) {
                        while (cur) {
                            st.stack[++st.top] = cur;
                            cur = cur->left;
                        }
                        cur = st.stack[st.top--];
                        node_count++;
                        cur = cur->right;
                    }
                }

                if (node_count == 0) {
                    printf("Ok: -1\n");
                    continue;
                }

                Node **nodes_array = malloc(node_count * sizeof(Node*));
                int idx = 0;

                {
                    struct {
                        Node* stack[1024];
                        int top;
                    } st;
                    st.top = -1;
                    Node *cur = root;
                    while (cur || st.top >= 0) {
                        while (cur) {
                            st.stack[++st.top] = cur;
                            cur = cur->left;
                        }
                        cur = st.stack[st.top--];
                        nodes_array[idx++] = cur;
                        cur = cur->right;
                    }
                }

                char unavailable_buf[1024];
                unavailable_buf[0] = '\0';
                int unavailable_count = 0;

                for (int i=0; i<node_count; i++) {
                    Node *n = nodes_array[i];
                    char reply[256];
                    if (send_request(context, n->endpoint, "ping", reply, sizeof(reply)) != 0 || strcmp(reply, "Ok") != 0) {
                        n->available = 0;
                        char tmp[32];
                        snprintf(tmp, sizeof(tmp), "%s%d", (unavailable_count > 0 ? ";" : ""), n->id);
                        strcat(unavailable_buf, tmp);
                        unavailable_count++;
                    } else {
                        n->available = 1;
                    }
                }

                free(nodes_array);

                if (unavailable_count == 0) {
                    printf("Ok: -1\n");
                } else {
                    printf("Ok: %s\n", unavailable_buf);
                }

            }

        } else if (strcmp(cmd, "exit") == 0) {
            break;
        } else {
            printf("Error: Unknown command\n");
        }
    }

    {
        struct {
            Node* stack[1024];
            int top;
        } st;
        st.top = -1;
        Node *cur = root;
        while (cur || st.top >= 0) {
            while (cur) {
                st.stack[++st.top] = cur;
                cur = cur->left;
            }
            cur = st.stack[st.top--];
            kill(cur->pid, SIGKILL);
            cur = cur->right;
        }
    }

    zmq_ctx_destroy(context);
    return 0;
}
