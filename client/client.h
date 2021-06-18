//
// Created by subhuman on 12.06.2021.
//

#ifndef SPOLAB15_CLIENT_H
#define SPOLAB15_CLIENT_H

#include <cypher-parser.h>
#include "../utils/linked_list.h"
#include "../utils/const.h"
#include "../utils/message.h"

int main(int argc, char **argv);

static void parse_response(char *response);

static query_info *get_query_info(cypher_parse_result_t *query);

static void set_labels_and_props(const cypher_astnode_t *node, linked_list *labels, linked_list *properties);

static void set_changed_labels_and_props(const cypher_astnode_t *node, query_info *info);
#endif //SPOLAB15_CLIENT_H
