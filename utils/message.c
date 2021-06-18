//
// Created by subhuman on 17.06.2021.
//

#include <string.h>
#include "message.h"

char *build_client_xml_request(query_info *queryInfo){
    xmlDocPtr doc = xmlNewDoc(BAD_CAST "1.0");
    xmlNodePtr request = xmlNewNode(NULL, BAD_CAST "request");
    xmlDocSetRootElement(doc, request);
    xmlNewProp(request, BAD_CAST "command", BAD_CAST queryInfo->command_type);
    xmlNodePtr first_node = xmlNewChild(request, NULL, BAD_CAST "node", NULL);
    xmlNodePtr labels = xmlNewChild(first_node, NULL, BAD_CAST "labels", NULL);
    xmlNodePtr props = xmlNewChild(first_node, NULL, BAD_CAST "props", NULL);
    build_xml_node_labels(labels, queryInfo->labels);
    build_xml_node_props(props, queryInfo->props);
    if(queryInfo->has_relation == true) {
        xmlNodePtr relation = xmlNewChild(request, NULL, BAD_CAST "relation", NULL);
        xmlNewProp(relation, BAD_CAST "name", BAD_CAST queryInfo->rel_name);
        xmlNodePtr second_node = xmlNewChild(request, NULL, BAD_CAST "node", NULL);
        xmlNodePtr rel_labels = xmlNewChild(second_node, NULL, BAD_CAST "labels", NULL);
        xmlNodePtr rel_props = xmlNewChild(second_node, NULL, BAD_CAST "props", NULL);
        build_xml_node_labels(rel_labels, queryInfo->rel_node_labels);
        build_xml_node_props(rel_props, queryInfo->rel_node_props);
    }
    if(strcmp(queryInfo->command_type, "set") == 0 || strcmp(queryInfo->command_type, "remove") == 0) {
        xmlNodePtr changes = xmlNewChild(request, NULL, BAD_CAST "changes", NULL);
        xmlNodePtr changed_labels = xmlNewChild(changes, NULL, BAD_CAST "labels", NULL);
        xmlNodePtr changed_props = xmlNewChild(changes, NULL, BAD_CAST "props", NULL);
        build_xml_node_labels(changed_labels, queryInfo->changed_labels);
        build_xml_node_props(changed_props, queryInfo->changed_props);
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

void free_query_info(query_info * info) {
    free_list(info->labels);
    free_list(info->props);
    free_list(info->rel_node_labels);
    free_list(info->rel_node_props);
    free_list(info->changed_labels);
    free_list(info->changed_props);
    free(info);
}


static void build_xml_node_labels(xmlNodePtr labels, linked_list *labels_list) {
    node *current_label = labels_list->first;
    for (int i = 0; i < labels_list->size; ++i) {
        xmlNodePtr label = xmlNewChild(labels, NULL, BAD_CAST "label", NULL);
        xmlNewProp(label, BAD_CAST "name", BAD_CAST current_label->value);
        current_label=current_label->next;
    }
}

static void build_xml_node_props(xmlNodePtr props, linked_list *props_list) {
    node* current_prop = props_list->first;
    for (int i = 0; i < props_list->size; ++i) {
        xmlNodePtr prop = xmlNewChild(props, NULL, BAD_CAST "prop", NULL);
        property *current = current_prop->value;
        xmlNewProp(prop, BAD_CAST "key", BAD_CAST current->key);
        xmlNewProp(prop, BAD_CAST "value", BAD_CAST current->value);
        current_prop=current_prop->next;
    }
}