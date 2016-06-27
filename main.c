#include <stdio.h>
#include <zmq.h>
#include <stdbool.h>
#include <stdlib.h>
#include <memory.h>
#include <getopt.h>

extern char *optarg;
extern int optind;

typedef struct list_t {
    const char *value;
    size_t len;
    struct list_t *next;
} list_t;

bool copy_list(void *zmq_sock, const list_t *list, bool finalize) {
    const list_t *ptr = list;
    while (ptr) {
        if (zmq_send(zmq_sock, ptr->value, ptr->len, ptr->next == NULL && finalize ? 0 : ZMQ_SNDMORE) == -1) {
            perror("send");
            return false;
        }
        ptr = ptr->next;
    }
    return true;
}

void append_list(list_t **list, const char *value, size_t len) {
    list_t *leaf;
    if (*list == NULL) {
        (*list) = (list_t *) malloc(sizeof(list_t));
        leaf = *list;
    } else {
        list_t *root = *list;
        while ((root->next)) {
            root = root->next;
        }
        leaf = (list_t *) malloc(sizeof(list_t));
        root->next = leaf;
    }
    leaf->len = len;
    leaf->value = value;
    leaf->next = NULL;

}

void clean_list(list_t **list) {
    list_t *root = *list, *tmp;
    while (root) {
        tmp = root;
        root = root->next;
        free(tmp);
    }
}

void summary(const char *command) {
    printf("zstream 1.0.0\n");
    printf("June 2016\n");
    printf("\n");
    printf("Usage:\n");
    printf("%s [-m mode][-s][-h][-l line-size][-t tokens][-a text][-p text] <endpoint>\n", command);
    printf("\n");
    printf("  endpoint     ZMQ endpoint string (like: tcp://localhost:9001)\n");
    printf("  -m mode      ZMQ socket mode: pub, sub (default: pub)\n");
    printf("  -l line-size positive integer as size in bytes of line buffer (default 65K)\n");
    printf("  -t tokens    characters which will be used as delimiters (same as in strtok)\n");
    printf("  -a text      append parts for each message (repeated field)\n");
    printf("  -p text      prepend parts for each message (repeated field)\n");
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
    list_t *prepend = NULL, *append = NULL;
    int rez = 0;
    while ((rez = getopt(argc, argv, "l:sm:t:ha:p:")) != -1) {
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
            case 'a':
                append_list(&append, optarg, strlen(optarg));
                break;
            case 'p':
                append_list(&prepend, optarg, strlen(optarg));
                break;
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
        //Prepend
        if (!copy_list(sock, prepend, false))
            break;
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
        //Append
        if (zmq_send(sock, prev, strlen(prev), append ? ZMQ_SNDMORE : 0) == -1) {
            perror("send");
            break;
        }
        if (!copy_list(sock, append, true))
            break;

    }
    free(line);
    clean_list(&append);
    clean_list(&prepend);
    zmq_ctx_destroy(context);
}