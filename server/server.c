//
// Created by subhuman on 14.06.2021.
//

#include <sys/socket.h>
#include <netinet/in.h>
#include <stddef.h>
#include <malloc.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
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

char *receive_message(char *client_message_part, int32_t accepted_socket) {
    long content_length = strtol(client_message_part, NULL, 10);

    if (content_length <= 0) {
        write(accepted_socket, "Bad request!", 12);
        return NULL;
    }

    printf("Content length: %lu bytes\n", content_length);

    char *request_xml = malloc(content_length);
    bzero(request_xml, content_length);

    long remain_data = content_length;
    while (remain_data > 0) {
        bzero(client_message_part, BUFSIZ);
        long len = recv(accepted_socket, client_message_part, BUFSIZ, 0);
        strcat(request_xml, client_message_part);
        remain_data -= len;
        printf("⬇ Received %ld bytes of request... Remaining: %ld\n", len, remain_data);
    }
    puts(request_xml);
    return request_xml;
}

void close_server(server_info *info) {
    close(info->server_fd);
}

static server_info *create_server_info(uint16_t port) {
    server_info *server = malloc(sizeof(server_info));
    server->port = port;
    return server;
}
