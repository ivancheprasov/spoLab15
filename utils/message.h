//
// Created by subhuman on 17.06.2021.
//

#ifndef SPOLAB15_MESSAGE_H
#define SPOLAB15_MESSAGE_H
#include "const.h"
#include "linked_list.h"
#include <libxml2/libxml/parser.h>

struct query_info {
    char command_type[COMMAND_TYPE_SIZE];
    linked_list *labels;
    linked_list *props;
    bool has_relation;
    char rel_name[RELATION_NAME_SIZE];
    linked_list *rel_node_labels;
    linked_list *rel_node_props;
    linked_list *changed_labels;
    linked_list *changed_props;
};

struct property {
    char key[PROPERTY_KEY_SIZE];
    char value[PROPERTY_VALUE_SIZE];
};

typedef struct query_info query_info;

typedef struct property property;

char *build_client_xml_request(query_info *queryInfo);

query_info *init_query_info();

void free_query_info(query_info * info);

static void build_xml_node_labels(xmlNodePtr labels, linked_list *labels_list);

static void build_xml_node_props(xmlNodePtr props, linked_list *props_list);
#endif //SPOLAB15_MESSAGE_H
