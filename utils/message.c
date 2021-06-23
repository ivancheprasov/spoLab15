#include <string.h>
#include "message.h"

char *build_client_xml_request(query_info *info) {
    xmlDocPtr doc = xmlNewDoc(BAD_CAST "1.0");
    xmlNodePtr request = xmlNewNode(NULL, BAD_CAST "request");
    xmlDocSetRootElement(doc, request);
    xmlNewProp(request, BAD_CAST "command", BAD_CAST info->command_type);
    xmlNodePtr first_node = xmlNewChild(request, NULL, BAD_CAST "node", NULL);
    xmlNodePtr labels = xmlNewChild(first_node, NULL, BAD_CAST "labels", NULL);
    xmlNodePtr props = xmlNewChild(first_node, NULL, BAD_CAST "props", NULL);
    build_xml_node_labels(labels, info->labels);
    build_xml_node_props(props, info->props);
    if (info->has_relation == true) {
        xmlNodePtr relation = xmlNewChild(request, NULL, BAD_CAST "relation", NULL);
        xmlNewProp(relation, BAD_CAST "name", BAD_CAST info->rel_name);
        xmlNodePtr second_node = xmlNewChild(request, NULL, BAD_CAST "node", NULL);
        xmlNodePtr rel_labels = xmlNewChild(second_node, NULL, BAD_CAST "labels", NULL);
        xmlNodePtr rel_props = xmlNewChild(second_node, NULL, BAD_CAST "props", NULL);
        build_xml_node_labels(rel_labels, info->rel_node_labels);
        build_xml_node_props(rel_props, info->rel_node_props);
    }
    if (strcmp(info->command_type, "set") == 0 || strcmp(info->command_type, "remove") == 0) {
        xmlNodePtr changes = xmlNewChild(request, NULL, BAD_CAST "changes", NULL);
        xmlNodePtr changed_labels = xmlNewChild(changes, NULL, BAD_CAST "labels", NULL);
        xmlNodePtr changed_props = xmlNewChild(changes, NULL, BAD_CAST "props", NULL);
        build_xml_node_labels(changed_labels, info->changed_labels);
        build_xml_node_props(changed_props, info->changed_props);
    }
    xmlChar *request_string;
    xmlDocDumpMemory(doc, &request_string, NULL);
    xmlFreeDoc(doc);
    return (char *) request_string;
}

void parse_xml_response(char *xml_request, char *response_string) {
    xmlDocPtr doc = xmlParseDoc(BAD_CAST xml_request);
    xmlNode *response = xmlDocGetRootElement(doc);
    xmlChar *command = xmlGetProp(response, BAD_CAST "command");
    char *command_type = (char *) command;
    xmlChar *number = xmlGetProp(response, BAD_CAST "number");
    if (strcmp(command_type, "match") == 0) {
        sprintf(response_string, "Number of matching nodes: %s", number);
        uint16_t offset = strlen(response_string);
        int32_t i = 0;
        for (xmlNode *node = response->children; node; node = node->next) {
            i++;
            xmlNode *labels = node->children;
            xmlNode *props = node->children->next;
            char labels_string[75];
            get_node_labels_string(labels, labels_string);
            char props_string[100];
            get_node_props_string(props, props_string);
            char node_string[200];
            bzero(node_string, 200);
            sprintf(node_string, "\n%d: labels (%s), props {%s}", i, labels_string, props_string);
            memcpy(response_string + offset, node_string, strlen(node_string));
            offset += strlen(node_string);
        }
    } else if (strcmp(command_type, "create") == 0 || strcmp(command_type, "delete") == 0) {
        char object_type_string[10];
        xmlChar *type = xmlGetProp(response, BAD_CAST "type");
        if (strcmp((char *) type, "node") == 0) {
            strcpy(object_type_string, "nodes");
        } else {
            strcpy(object_type_string, "relations");
        }
        char command_type_string[10];
        if (strcmp(command_type, "create") == 0) {
            strcpy(command_type_string, "created");
        } else {
            strcpy(command_type_string, "deleted");
        }
        sprintf(response_string, "Number of %s %s: %s", object_type_string, command_type_string, number);
    } else if (strcmp(command_type, "set") == 0 || strcmp(command_type, "remove") == 0) {
        xmlNode *child = response->children;
        char command_type_string[10];
        if (strcmp(command_type, "set") == 0) {
            strcpy(command_type_string, "set");
        } else {
            strcpy(command_type_string, "removed");
        }
        if (strcmp((char *) child->name, "labels") == 0) {
            char labels_string[BUFSIZ];
            get_node_labels_string(child, labels_string);
            sprintf(response_string, "Number of nodes with labels (%s) %s: %s", labels_string, command_type_string,
                    number);
        } else if (strcmp((char *) child->name, "props") == 0) {
            xmlNode *prop = child->children;
            xmlChar *key = xmlGetProp(prop, BAD_CAST "key");
            if (strcmp(command_type, "set") == 0) {
                char prop_string[BUFSIZ];
                bzero(prop_string, BUFSIZ);
                xmlChar *value = xmlGetProp(prop, BAD_CAST "value");
                sprintf(prop_string, "%s=%s", key, value);
                sprintf(response_string, "Number of nodes with attribute '%s' %s: %s", prop_string, command_type_string,
                        number);
            } else {
                sprintf(response_string, "Number of nodes with attribute '%s' %s: %s", key, command_type_string,
                        number);
            }
        } else {
            sprintf(response_string, "Nothing changed");
        }
    }
}

query_info *init_query_info() {
    query_info *info = malloc(sizeof(query_info));
    info->labels = init_list();
    info->props = init_list();
    info->rel_node_labels = init_list();
    info->rel_node_props = init_list();
    info->changed_labels = init_list();
    info->changed_props = init_list();
    info->has_relation = false;
    return info;
}

match_result *init_match_result() {
    match_result *match = malloc(sizeof(match_result));
    match->labels = init_list();
    match->props = init_list();
    return match;
}

void free_query_info(query_info *info) {
    free_list(info->labels);
    free_list(info->props);
    free_list(info->rel_node_labels);
    free_list(info->rel_node_props);
    free_list(info->changed_labels);
    free_list(info->changed_props);
    free(info);
}

query_info *parse_client_xml_request(char *xml_request) {
    xmlDocPtr doc = xmlParseDoc(BAD_CAST xml_request);
    if (doc == NULL) return NULL;
    xmlNode *request = xmlDocGetRootElement(doc);
    if (request == NULL) return NULL;
    xmlChar *command = xmlGetProp(request, BAD_CAST "command");
    if (command == NULL) return NULL;
    query_info *info = init_query_info();
    strcpy(info->command_type, (char *) command);
    xmlNode *current_node = request->children;
    xmlNode *node_labels = current_node->children;
    if (parse_xml_node_labels(node_labels, info->labels) == NULL) return NULL;
    xmlNode *props = node_labels->next;
    if (parse_xml_node_props(props, info->props) == NULL) return NULL;
    current_node = current_node->next;
    if (current_node == NULL) return info;
    if (strcmp((char *) current_node->name, "relation") == 0) {
        xmlChar *name = xmlGetProp(current_node, BAD_CAST "name");
        if (name == NULL) return NULL;
        strcpy(info->rel_name, (char *) name);
        info->has_relation = true;
        current_node = current_node->next;
        if (current_node == NULL) return NULL;
        node_labels = current_node->children;
        if (parse_xml_node_labels(node_labels, info->rel_node_labels) == NULL) return NULL;
        props = node_labels->next;
        if (parse_xml_node_props(props, info->rel_node_props) == NULL) return NULL;
        current_node = current_node->next;
    }
    if (current_node == NULL) return info;
    if (strcmp((char *) current_node->name, "changes") == 0) {
        xmlNode *changed_labels = current_node->children;
        if (parse_xml_node_labels(changed_labels, info->changed_labels) == NULL) return NULL;
        xmlNode *changed_props = changed_labels->next;
        if (parse_xml_node_props(changed_props, info->changed_props) == NULL) return NULL;
    }
    return info;
}

char *build_xml_match_response(linked_list *match_results, uint16_t number) {
    char buffer[BUFSIZ];
    sprintf(buffer, "%hu", number);
    xmlDocPtr doc = xmlNewDoc(BAD_CAST "1.0");
    xmlNodePtr response = xmlNewNode(NULL, BAD_CAST "response");
    xmlDocSetRootElement(doc, response);
    xmlNewProp(response, BAD_CAST "command", BAD_CAST "match");
    xmlNewProp(response, BAD_CAST "number", BAD_CAST buffer);
    for (node *matching_node = match_results->first; matching_node; matching_node = matching_node->next) {
        xmlNodePtr node = xmlNewChild(response, NULL, BAD_CAST "node", NULL);
        xmlNodePtr labels = xmlNewChild(node, NULL, BAD_CAST "labels", NULL);
        xmlNodePtr props = xmlNewChild(node, NULL, BAD_CAST "props", NULL);
        build_xml_node_labels(labels, ((match_result *) matching_node->value)->labels);
        build_xml_node_props(props, ((match_result *) matching_node->value)->props);
    }
    xmlChar *response_string;
    xmlDocDumpMemory(doc, &response_string, NULL);
    xmlFreeDoc(doc);
    return (char *) response_string;
}

char *build_xml_create_or_delete_response(char *command_type, char *object_type, uint16_t number) {
    char buffer[BUFSIZ];
    sprintf(buffer, "%hu", number);
    xmlDocPtr doc = xmlNewDoc(BAD_CAST "1.0");
    xmlNodePtr response = xmlNewNode(NULL, BAD_CAST "response");
    xmlDocSetRootElement(doc, response);
    xmlNewProp(response, BAD_CAST "command", BAD_CAST command_type);
    xmlNewProp(response, BAD_CAST "type", BAD_CAST object_type);
    xmlNewProp(response, BAD_CAST "number", BAD_CAST buffer);
    xmlChar *response_string;
    xmlDocDumpMemory(doc, &response_string, NULL);
    xmlFreeDoc(doc);
    return (char *) response_string;
}

char *build_xml_set_or_remove_response(char *command_type, char *object_type, linked_list *changed, uint16_t number) {
    char buffer[BUFSIZ];
    sprintf(buffer, "%hu", number);
    xmlDocPtr doc = xmlNewDoc(BAD_CAST "1.0");
    xmlNodePtr response = xmlNewNode(NULL, BAD_CAST "response");
    xmlDocSetRootElement(doc, response);
    xmlNewProp(response, BAD_CAST "command", BAD_CAST command_type);
    xmlNodePtr child = xmlNewChild(response, NULL, BAD_CAST object_type, NULL);
    if (strcmp(object_type, "labels") == 0) {
        build_xml_node_labels(child, changed);
    } else {
        build_xml_node_props(child, changed);
    }
    xmlNewProp(response, BAD_CAST "number", BAD_CAST buffer);
    xmlChar *response_string;
    xmlDocDumpMemory(doc, &response_string, NULL);
    xmlFreeDoc(doc);
    return (char *) response_string;
}

bool by_property_values(void *value, char *to_find_key, char* to_find_value) {
    return strcmp(((property *)value)->key, to_find_key) == 0 && strcmp(((property *)value)->value, to_find_value) == 0;
}

bool by_key(void *value, char *key, char *second_value) {
    return strcmp(((property *)value)->key, key) == 0;
}

static void *parse_xml_node_labels(xmlNode *node_labels, linked_list *labels) {
    for (xmlNode *label = node_labels->children; label; label = label->next) {
        xmlChar *name = xmlGetProp(label, BAD_CAST "name");
        if (name == NULL) return NULL;
        add_last(labels, (char *) name);
    }
    return labels;
}

static void *parse_xml_node_props(xmlNode *node_props, linked_list *props) {
    for (xmlNode *prop = node_props->children; prop; prop = prop->next) {
        property *current_prop = malloc(sizeof(property));
        xmlChar *key = xmlGetProp(prop, BAD_CAST "key");
        xmlChar *value = xmlGetProp(prop, BAD_CAST "value");
        if (key == NULL || value == NULL) return NULL;
        bzero(current_prop->key, strlen((current_prop->key)));
        bzero(current_prop->value, strlen((current_prop->value)));
        strcpy(current_prop->key, (char *) key);
        strcpy(current_prop->value, (char *) value);
        add_last(props, current_prop);
    }
    return node_props;
}

static void build_xml_node_labels(xmlNodePtr labels, linked_list *labels_list) {
    node *current_label = labels_list->first;
    for (int i = 0; i < labels_list->size; ++i) {
        xmlNodePtr label = xmlNewChild(labels, NULL, BAD_CAST "label", NULL);
        xmlNewProp(label, BAD_CAST "name", BAD_CAST current_label->value);
        current_label = current_label->next;
    }
}

static void build_xml_node_props(xmlNodePtr props, linked_list *props_list) {
    node *current_prop = props_list->first;
    for (int i = 0; i < props_list->size; ++i) {
        xmlNodePtr prop = xmlNewChild(props, NULL, BAD_CAST "prop", NULL);
        property *current = current_prop->value;
        xmlNewProp(prop, BAD_CAST "key", BAD_CAST current->key);
        xmlNewProp(prop, BAD_CAST "value", BAD_CAST current->value);
        current_prop = current_prop->next;
    }
}

static void get_node_labels_string(xmlNode *labels, char *labels_string) {
    bzero(labels_string, strlen(labels_string));
    xmlNode *label = labels->children;
    if(label != NULL) {
        xmlChar *name = xmlGetProp(label, BAD_CAST "name");
        sprintf(labels_string, "%s", name);
        uint32_t offset = 0;
        for (label = label->next; label; label = label->next) {
            offset += strlen((char *) name);
            name = xmlGetProp(label, BAD_CAST "name");
            memcpy(labels_string + offset, ", ", 2);
            offset += 2;
            char label_name_str[strlen((char *) name)];
            bzero(label_name_str, strlen(label_name_str));
            memcpy(label_name_str, (char *) name, strlen((char *) name));
            memcpy(labels_string + offset, label_name_str, strlen(label_name_str));
        }
    }
}

static void get_node_props_string(xmlNode *props, char *props_string) {
    bzero(props_string, strlen(props_string));
    xmlNode *prop = props->children;
    if(prop != NULL) {
        xmlChar *key = xmlGetProp(prop, BAD_CAST "key");
        xmlChar *value = xmlGetProp(prop, BAD_CAST "value");
        sprintf(props_string, "%s=%s", key, value);
        uint32_t offset = 0;
        for (prop = prop->next; prop; prop = prop->next) {
            offset += strlen((char *) key)+1+strlen((char *) value);
            key = xmlGetProp(prop, BAD_CAST "key");
            value = xmlGetProp(prop, BAD_CAST "value");
            memcpy(props_string + offset, ", ", 2);
            offset += 2;
            char prop_str[strlen((char *) key)+1+strlen((char *) value)];
            bzero(prop_str, strlen(prop_str));
            sprintf(prop_str, "%s=%s", (char *) key, (char *) value);
            memcpy(props_string + offset, prop_str, strlen(prop_str));
        }
    }
}