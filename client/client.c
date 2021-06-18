#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <cypher-parser.h>
#include "client.h"

int main(int argc, char **argv) {
    if (argc < 2) {
        puts("No required arguments provided: <server_port>");
        return -1;
    }
    uint16_t port = strtoul(argv[1], NULL, BASE_10);
    int32_t server_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    server_address.sin_addr.s_addr = inet_addr(HOSTNAME);

    int connection = connect(server_fd, (const struct sockaddr *) &server_address, sizeof(server_address));
    if (connection == -1) {
        puts("There was an error making a connection to the remote socket");
        return -1;
    }
    char *input = NULL;
    size_t length;
    uint8_t errors;
    puts("Enter CYPHER query or type \"exit\" to leave");
    long count = server_fd;
    while (count > 0) {
        getline(&input, &length, stdin);
        cypher_parse_result_t *result = cypher_parse(
                input, NULL, NULL, CYPHER_PARSE_ONLY_STATEMENTS);
        if (result == NULL) {
            perror("cypher_parse");
            return EXIT_FAILURE;
        }
        cypher_parse_result_fprint_ast(result, stdout, 100, NULL, 0);
        errors = cypher_parse_result_nerrors(result);
        if (errors > 0) {
            puts("Unknown command");
            cypher_parse_result_free(result);
            continue;
        }
        query_info *info = get_query_info(result);
        char *request = build_client_xml_request(info);
        puts(request);
        free_query_info(info);
        cypher_parse_result_free(result);
    }
}

static query_info *get_query_info(cypher_parse_result_t *parsing_result) {
    query_info *query_info = init_query_info();
    const cypher_astnode_t *ast = cypher_parse_result_get_directive(parsing_result, 0);
    const cypher_astnode_t *query = cypher_ast_statement_get_body(ast);
    const cypher_astnode_t *clause = cypher_ast_query_get_clause(query, 0);
    cypher_astnode_type_t command = cypher_astnode_type(clause);
    if (command == CYPHER_AST_MATCH) {
        const cypher_astnode_t *clause1 = cypher_ast_query_get_clause(query, 1);
        if (clause1 == NULL) {
            puts("Unknown command");
            return NULL;
        }
        command = cypher_astnode_type(clause1);
        if (command == CYPHER_AST_RETURN) {
            memcpy(query_info->command_type, "match", COMMAND_TYPE_SIZE);
        }
        if (command == CYPHER_AST_SET) {
            memcpy(query_info->command_type, "set", COMMAND_TYPE_SIZE);
        }
        if (command == CYPHER_AST_REMOVE) {
            memcpy(query_info->command_type, "remove", COMMAND_TYPE_SIZE);
        }
        if (command == CYPHER_AST_DELETE) {
            memcpy(query_info->command_type, "delete", COMMAND_TYPE_SIZE);
        }
        set_changed_labels_and_props(clause1, query_info);
    }
    if (command == CYPHER_AST_CREATE) {
        memcpy(query_info->command_type, "create", COMMAND_TYPE_SIZE);
    }
    cypher_astnode_t *pattern;
    if (cypher_astnode_type(clause) == CYPHER_AST_MATCH) {
        pattern = cypher_ast_match_get_pattern(clause);
    } else {
        pattern = cypher_ast_create_get_pattern(clause);
    }
    const cypher_astnode_t *path = cypher_ast_pattern_get_path(pattern, 0);
    const cypher_astnode_t *node = cypher_ast_pattern_path_get_element(path, 0);
    set_labels_and_props(node, query_info->labels, query_info->props);
    if (cypher_ast_pattern_path_nelements(path) == 3) {
        query_info->has_relation = true;
        const cypher_astnode_t *relation = cypher_ast_pattern_path_get_element(path, 1);
        const cypher_astnode_t *rel_type = cypher_ast_rel_pattern_get_reltype(relation, 0);
        memcpy(query_info->rel_name, cypher_ast_reltype_get_name(rel_type), RELATION_NAME_SIZE);
        const cypher_astnode_t *rel_node = cypher_ast_pattern_path_get_element(path, 2);
        set_labels_and_props(rel_node, query_info->rel_node_labels, query_info->rel_node_props);
    }
    return query_info;
}

static void set_labels_and_props(const cypher_astnode_t *node, linked_list *labels, linked_list *properties) {
    for (int i = 0; i < cypher_ast_node_pattern_nlabels(node); ++i) {
        const cypher_astnode_t *label = cypher_ast_node_pattern_get_label(node, i);
        add_last(labels, (void *) cypher_ast_label_get_name(label));
    }
    const cypher_astnode_t *props = cypher_ast_node_pattern_get_properties(node);
    if (props != NULL) {
        for (int i = 0; i < cypher_ast_map_nentries(props); ++i) {
            const cypher_astnode_t *key = cypher_ast_map_get_key(props, i);
            const cypher_astnode_t *value = cypher_ast_map_get_value(props, i);
            property *prop = malloc(sizeof(property));
            memset(prop, 0, PROPERTY_KEY_SIZE + PROPERTY_VALUE_SIZE);
            if (
                    strlen(cypher_ast_prop_name_get_value(key)) > PROPERTY_KEY_SIZE ||
                    cypher_astnode_type(value) != CYPHER_AST_STRING ||
                    strlen(cypher_ast_string_get_value(value)) > PROPERTY_VALUE_SIZE
                    ) {
                printf("Invalid Property. Max key size = %d. Max value size = %d. Only string properties allowed.",
                       PROPERTY_KEY_SIZE, PROPERTY_VALUE_SIZE);
            } else {
                strcpy(prop->key, cypher_ast_prop_name_get_value(key));
                strcpy(prop->value, cypher_ast_string_get_value(value));
                add_last(properties, prop);
            }
        }
    }
}

static void set_changed_labels_and_props(const cypher_astnode_t *clause, query_info *info) {
    if (cypher_astnode_type(clause) == CYPHER_AST_SET) {
        const cypher_astnode_t *item = cypher_ast_set_get_item(clause, 0);
        if (cypher_astnode_type(item) == CYPHER_AST_SET_LABELS) {
            for (int i = 0; i < cypher_ast_set_labels_nlabels(item); ++i) {
                const cypher_astnode_t *set_label = cypher_ast_set_labels_get_label(item, i);
                add_last(info->changed_labels, (void *) cypher_ast_label_get_name(set_label));
            }
        }
        if (cypher_astnode_type(item) == CYPHER_AST_SET_PROPERTY) {
            const cypher_astnode_t *set_prop = cypher_ast_set_property_get_property(item);
            const cypher_astnode_t *prop_name = cypher_ast_property_operator_get_prop_name(set_prop);
            const cypher_astnode_t *expression = cypher_ast_set_property_get_expression(item);
            property *prop = malloc(sizeof(property));
            if (
                    strlen(cypher_ast_prop_name_get_value(prop_name)) >
                    PROPERTY_KEY_SIZE ||
                    cypher_astnode_type(expression) != CYPHER_AST_STRING ||
                    strlen(cypher_ast_string_get_value(expression)) > PROPERTY_VALUE_SIZE
                    ) {
                printf("Invalid Property. Only string properties allowed.");
            } else {
                strcpy(prop->key, cypher_ast_prop_name_get_value(prop_name));
                strcpy(prop->value, cypher_ast_string_get_value(expression));
                add_last(info->changed_props, prop);
            }
        }
    }
    if (cypher_astnode_type(clause) == CYPHER_AST_REMOVE) {
        const cypher_astnode_t *item = cypher_ast_remove_get_item(clause, 0);
        if (cypher_astnode_type(item) == CYPHER_AST_REMOVE_LABELS) {
            for (int i = 0; i < cypher_ast_remove_labels_nlabels(item); ++i) {
                const cypher_astnode_t *set_label = cypher_ast_remove_labels_get_label(item, i);
                add_last(info->changed_labels, (void *) cypher_ast_label_get_name(set_label));
            }
        }
        if (cypher_astnode_type(item) == CYPHER_AST_REMOVE_PROPERTY) {
            const cypher_astnode_t *remove_prop = cypher_ast_remove_property_get_property(item);
            const cypher_astnode_t *prop_name = cypher_ast_property_operator_get_prop_name(remove_prop);
            property *prop = malloc(sizeof(property));
            if (
                    strlen(cypher_ast_prop_name_get_value(prop_name)) >
                    PROPERTY_KEY_SIZE) {
                printf("Invalid Property. Max key size = %d",
                       PROPERTY_KEY_SIZE);
            } else {
                strcpy(prop->key, cypher_ast_prop_name_get_value(prop_name));
                add_last(info->changed_props, prop);
            }
        }
    }
}