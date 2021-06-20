//
// Created by subhuman on 12.06.2021.
//

#ifndef SPOLAB15_CLIENT_H
#define SPOLAB15_CLIENT_H

#include <cypher-parser.h>
#include "../utils/linked_list.h"
#include "../utils/const.h"
#include "../utils/message.h"

void send_message(int32_t server_fd, char* request);

query_info *get_query_info(cypher_parse_result_t *query);

char* receive_message(int32_t server_fd, long content_length);

static void set_labels_and_props(const cypher_astnode_t *node, linked_list *labels, linked_list *properties);

static void set_changed_labels_and_props(const cypher_astnode_t *node, query_info *info);

static void handle_sending_error();
#endif //SPOLAB15_CLIENT_H
