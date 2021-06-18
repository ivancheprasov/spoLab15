//
// Created by subhuman on 14.06.2021.
//

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdbool.h>
#include "../utils/const.h"
#include "server.h"

int main(int argc, char **argv) {
    if (argc < 3) {
        puts("No required arguments provided: <server_port> <data_file>");
        return -1;
    }
    uint16_t port = strtoul(argv[1], NULL, BASE_10);

    server_info *info_ptr = startup(port);

    if (info_ptr == NULL) {
        puts("Unable to startup");
        return -1;
    }

    char buffer [MAX_MSG_SIZE];
    struct sockaddr_in client_address;
    socklen_t address_len = sizeof(client_address);
    int32_t accepted_socket = accept(info_ptr->server_fd, (struct sockaddr *) &client_address, &address_len);
    while(true) {
        recv(accepted_socket, buffer, MAX_MSG_SIZE, MSG_NOSIGNAL);
    }
    close_server(info_ptr);
    free(info_ptr);
}