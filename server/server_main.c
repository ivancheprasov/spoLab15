#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include "../utils/const.h"
#include "server.h"
#include "../datafile/datafile.h"

int main(int argc, char **argv) {
    if (argc < 3) {
        puts("No required arguments provided: <server_port> <data_file>");
        return -1;
    }
    FILE* data_file = open_data_file(argv[2]);
    fclose(data_file);
    uint16_t port = strtoul(argv[1], NULL, BASE_10);
    server_info *info_ptr = startup(port);

    if (info_ptr == NULL) {
        puts("Unable to startup");
        return -1;
    }

    struct sockaddr_in client_address;
    socklen_t address_len = sizeof(client_address);
    int32_t accepted_socket = accept(info_ptr->server_fd, (struct sockaddr *) &client_address, &address_len);
    char client_message_part[BUFSIZ] = {0};

    while (recv(accepted_socket, client_message_part, BUFSIZ, 0) > 0) {
        char *request_xml = receive_message(client_message_part, accepted_socket);
        if (request_xml == NULL) continue;
        query_info *info = parse_client_xml_request(request_xml);
        char *response_xml = execute_command(info);
        puts(response_xml);
        free(request_xml);
        char *response_header = malloc(BUFSIZ);
        bzero(response_header, BUFSIZ);
        sprintf(response_header, "%lu", strlen(response_xml));
        if (write(accepted_socket, response_header, BUFSIZ) < 0) break;
        free(response_header);
        send_message(accepted_socket, response_xml);
        free(response_xml);
        bzero(client_message_part, BUFSIZ);
    }
    close_server(info_ptr);
    free(info_ptr);
}