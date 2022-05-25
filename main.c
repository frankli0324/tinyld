#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/auxv.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "tinyld.h"

struct bd_t *blob_fds[64] __attribute__((visibility("default")));

int bind_unix(char *path) {
    int server_sockfd;
    struct sockaddr_un local;
    int len;

    if ((server_sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("Error creating server socket");
        exit(1);
    }

    local.sun_family = AF_UNIX;
    strcpy(local.sun_path, path);
    unlink(local.sun_path);
    len = strlen(local.sun_path) + sizeof(local.sun_family);
    if (bind(server_sockfd, (struct sockaddr *)&local, len) == -1) {
        perror("binding");
        exit(1);
    }
    return server_sockfd;
}

void fork_handler(int client_sockfd) {
    char buffer[1024];
    char *data = malloc(40960);
    // char *argv[] = {"test"};
    unsigned int len, total = 0;

    printf("Handling connection\n");
    fflush(stdout);

    while ((len = recv(client_sockfd, &buffer, 1024, 0)) > 0) {
        memcpy(data + total, &buffer, len);
        total += len;
    }
    close(client_sockfd);
    printf("Got blob[%d], loading\n", total);
    void *handle = t_dlmopen(data, total, RTLD_NOW);
    int (*execute)(int, char **) = t_dlsym(handle, "execute");
    printf("entry point at %p\n", execute);
    execute(0, NULL);
    t_dlclose(handle);
    exit(0);
}

int main(int argc, char **argv) {
    if (argc < 2)
        return 0;
    int client_sockfd;
    struct sockaddr_un remote;
    int srv = bind_unix(argv[1]);

    if (listen(srv, 5) == -1) {
        perror("listen");
        exit(1);
    }

    printf("Listening...\n");
    fflush(stdout);

    while (1) {
        printf("Waiting for a connection\n");
        fflush(stdout);
        socklen_t remote_len = sizeof(remote);
        if ((client_sockfd = accept(srv, (struct sockaddr *)&remote, &remote_len)) == -1) {
            perror("accept");
            exit(1);
        }

        printf("Accepted connection\n");
        fflush(stdout);
        switch (fork()) {
        case -1:
            perror("Error forking connection handler");
            break;
        case 0:
            fork_handler(client_sockfd);
            exit(0);
        default:
            break;
        }
    }
    return 0;
}
