#include <stdio.h>
#include <zmq.h>
#include <stdbool.h>
#include <stdlib.h>
#include <memory.h>
#include <getopt.h>

int main(int argc, const char **argv) {
    const char *endpoint = "tcp://*:9001";
    int max_line_size = 65535;
    bool is_server = true;
    int type = ZMQ_PUB; //ZMQ_PUSH
    const char *tokens = " ";
    void *context = zmq_ctx_new();
    if (!context) {
        perror("new context");
        return 1;
    }
    void *sock = zmq_socket(context, type);
    if (!sock) {
        perror("socket");
        zmq_ctx_destroy(context);
        return 1;
    }
    if (is_server) {
        if (zmq_bind(sock, endpoint) != 0) {
            perror("bind");
            zmq_ctx_destroy(context);
            return 1;
        }
    } else {
        if (zmq_connect(sock, endpoint) != 0) {
            perror("connect");
            zmq_ctx_destroy(context);
            return 1;
        }
    }
    char *line = (char *) malloc((size_t) max_line_size);
    while (!feof(stdin)) {
        char *text = fgets(line, max_line_size, stdin);
        if (!text) break;
        size_t text_len = strlen(text);

        if (text[text_len - 1] == '\n') { // Trim
            text[text_len - 1] = '\0';
            --text_len;
        }

        if (text_len == 0) // Empty line
            continue;

        char *part, *prev = NULL;
        while ((part = strtok(text, tokens))) {
            text = NULL;
            if (prev) {
                size_t prev_size = strlen(prev);
                if (zmq_send(sock, prev, prev_size, ZMQ_SNDMORE) == -1) {
                    perror("send");
                    break;
                }
            }
            prev = part;
        }
        if (prev) {
            if (zmq_send(sock, prev, strlen(prev), 0) == -1) {
                perror("send");
                break;
            }
        }

    }
    free(line);
    zmq_ctx_destroy(context);
}