//
// Created by subhuman on 14.06.2021.
//

#ifndef SPOLAB15_SERVER_H
#define SPOLAB15_SERVER_H

#include <stdint.h>

struct server_info {
    uint16_t port;
    int32_t server_fd;
};

typedef struct server_info server_info;

server_info *startup(uint16_t port);

void close_server(server_info *info);

static server_info *create_server_info(uint16_t port);

#endif //SPOLAB15_SERVER_H
