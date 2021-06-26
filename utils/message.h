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

struct match_result {
    linked_list *labels;
    linked_list *props;
};

typedef struct query_info query_info;

typedef struct property property;

typedef struct match_result match_result;

char *build_client_xml_request(query_info *info);

query_info *parse_client_xml_request(char *xml_request);

void parse_xml_response(char *xml_request, char *response_string);

query_info *init_query_info();

match_result *init_match_result();

void free_query_info(query_info * info);

char *build_xml_match_response(linked_list *match_results, uint64_t number);

char *build_xml_create_or_delete_response(char *command_type, char *object_type, uint64_t number);

char *build_xml_set_or_remove_response(char *command_type, char *object_type, linked_list *changed, uint64_t number);

void free_match_result(linked_list *match_results);

bool by_property_values(void *value, char *to_find_key, char* to_find_value);

static void *parse_xml_node_labels(xmlNode *node_labels, linked_list *labels);

static void *parse_xml_node_props(xmlNode *node_props, linked_list *props);

static void build_xml_node_labels(xmlNodePtr labels, linked_list *labels_list);

static void build_xml_node_props(xmlNodePtr props, linked_list *props_list);

static void get_node_labels_string(xmlNode *labels, char *labels_string);

static void get_node_props_string(xmlNode *props, char *props_string);
#endif //SPOLAB15_MESSAGE_H
