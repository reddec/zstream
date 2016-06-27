#include <stdio.h>
#include <zmq.h>
#include <stdbool.h>
#include <stdlib.h>
#include <memory.h>
#include <getopt.h>

extern char *optarg;
extern int optind;

void summary(const char *command) {
    printf("zstream 1.0.0\n");
    printf("June 2016\n");
    printf("\n");
    printf("Usage:\n");
    printf("%s [-m mode][-s][-h][-l line-size][-t tokens] <endpoint>\n", command);
    printf("\n");
    printf("  endpoint     ZMQ endpoint string (like: tcp://localhost:9001)\n");
    printf("  -m mode      ZMQ socket mode: pub, sub (default: pub)\n");
    printf("  -l line-size positive integer as size in bytes of line buffer (default 65K)\n");
    printf("  -t tokens    characters which will be used as delimiters (same as in strtok)\n");
    printf("  -s           become as server (bind and listen)\n");
    printf("  -h           show this help and exit\n");
    printf("\n");
    printf("Copyright: zstream 2016 Baryshnikov Alexander <dev@baryshnikov.net>\n");
}

int main(int argc, char *const *argv) {
    int max_line_size = 65535;
    bool is_server = false;
    int type = ZMQ_PUB; //ZMQ_PUSH
    char *tokens = " ";
    char *endpoint;

    int rez = 0;
    while ((rez = getopt(argc, argv, "l:cm:t:h")) != -1) {
        switch (rez) {
            case 'l':
                max_line_size = atoi(optarg);
                if (max_line_size <= 0) {
                    fprintf(stderr, "line size must have positive value\n");
                    return 1;
                }
                break;
            case 's':
                is_server = true;
                break;
            case 'm':
                if (strcmp(optarg, "pub") == 0)
                    type = ZMQ_PUB;
                else if (strcmp(optarg, "push") == 0)
                    type = ZMQ_PUSH;
                else {
                    fprintf(stderr, "invalid mode\n");
                    return 1;
                }
                break;
            case 't':
                tokens = optarg;
                break;
            case 'h':
                summary(argv[0]);
                return 0;
            default:
                summary(argv[0]);
                return 1;
        };
    };
    if (optind >= argc) {
        fprintf(stderr, "required parameter endpoint not provided\n");
        summary(argv[0]);
        return 1;
    }
    endpoint = argv[optind];


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