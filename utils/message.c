//
// Created by subhuman on 17.06.2021.
//

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