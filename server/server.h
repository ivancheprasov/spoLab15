#ifndef SPOLAB15_SERVER_H
#define SPOLAB15_SERVER_H

#include <stdint.h>
#include "../utils/message.h"

struct server_info {
    uint16_t port;
    int32_t server_fd;
};

typedef struct server_info server_info;

server_info *startup(uint16_t port);

char *receive_message(char *client_message_part, int32_t accepted_socket);

void send_message(int32_t client_socket, char *message);

void close_server(server_info *info);

char *execute_command(query_info *info);

static server_info *create_server_info(uint16_t port);


#endif //SPOLAB15_SERVER_H
