//
// Created by subhuman on 14.06.2021.
//

#include <sys/socket.h>
#include <netinet/in.h>
#include <stddef.h>
#include <malloc.h>
#include <unistd.h>
#include "server.h"

server_info *startup(uint16_t port) {
    server_info *server_info_ptr = create_server_info(port);
    int created_socket = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in address;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(server_info_ptr->port);
    address.sin_family = AF_INET;
    server_info_ptr->server_fd = created_socket;
    int bind_result = bind(created_socket, (const struct sockaddr *) &address, sizeof(address));
    if (bind_result == -1) {
        return NULL;
    }
    listen(created_socket, 1);
//    pthread_create(&server_info_ptr->manager_t_id, NULL, (void *(*)(void *)) manage_connections, server_info_ptr);
    return server_info_ptr;
}

void close_server(server_info *info) {
    close(info->server_fd);
}

static server_info *create_server_info(uint16_t port) {
    server_info *server = malloc(sizeof(server_info));
    server->port = port;
    return server;
}
