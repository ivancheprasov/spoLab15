#include <sys/socket.h>
#include <netinet/in.h>
#include <stddef.h>
#include <malloc.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "server.h"

server_info *startup(uint16_t port) {
    server_info *server_info_ptr = create_server_info(port);
    int created_socket = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in address;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(server_info_ptr->port);
    address.sin_family = AF_INET;
    server_info_ptr->server_fd = created_socket;
    int bind_result = bind(created_socket, (const struct sockaddr *) &address, sizeof(address));
    if (bind_result == -1) {
        return NULL;
    }
    listen(created_socket, 1);
//    pthread_create(&server_info_ptr->manager_t_id, NULL, (void *(*)(void *)) manage_connections, server_info_ptr);
    return server_info_ptr;
}

char *receive_message(char *client_message_part, int32_t accepted_socket) {
    long content_length = strtol(client_message_part, NULL, 10);
    if (content_length <= 0) {
        write(accepted_socket, "Bad request!", 12);
        return NULL;
    }
    printf("Content length: %lu bytes\n", content_length);
    char *request_xml = malloc(content_length);
    bzero(request_xml, content_length);
    long remain_data = content_length;
    while (remain_data > 0) {
        bzero(client_message_part, BUFSIZ);
        long len = recv(accepted_socket, client_message_part, BUFSIZ, 0);
        strcat(request_xml, client_message_part);
        remain_data -= len;
        printf("⬇ Received %ld bytes of request... Remaining: %ld\n", len, remain_data);
    }
    puts(request_xml);
    return request_xml;
}

void send_message(int32_t client_socket, char *message) {
    long remain_data = (long) strlen(message);
    int offset = 0;
    do {
        int packet_size = remain_data > BUFSIZ ? BUFSIZ : (int) remain_data;
        char *data = malloc(packet_size);
        memcpy(data, message + offset, packet_size);
        offset += packet_size;
        if (write(client_socket, data, packet_size) < 0) break;
        free(data);
        remain_data -= packet_size;
        printf("⬆ Sent %d bytes of response... Remaining: %lu\n", packet_size, remain_data);
    } while (remain_data > 0);
}

char *execute_command(query_info *info, datafile *data) {
    char *command = info->command_type;
    uint16_t number = 0;
    if (strcmp(command, "match") == 0) {
        //toDo возврат всех совпадений (всех аттрибутов и меток) без списка отношений, подсчёт кол-ва найденных узлов
        linked_list *match_results = init_list();
        linked_list *node_ptr = init_list();
        number = match(info, data, node_ptr, match_results);
        return build_xml_match_response(match_results, number);
    }
    if (strcmp(command, "create") == 0) {
        if (info->has_relation) {
            //toDo сравнение с заданным шаблоном, создание указанной связи, подсчёт кол-ва созданных связей
            return build_xml_create_or_delete_response("create", "relation", number);
        } else {
            number = 1;
            //toDo создание узла с заданными параметрами
            cell_ptr *node_cell = create_node_cell(data);
            if (info->labels->size > 0) {
                for (node *label = info->labels->first; label; label = label->next) {
                    cell_ptr *string_cell = create_string_cell(data, label->value);
                    cell_ptr *label_cell = create_label_cell(data, string_cell, node_cell);
                    update_node_labels(data, node_cell, label_cell);
                }
            }
            return build_xml_create_or_delete_response("create", "node", number);
        }
    }
    if (strcmp(command, "delete") == 0) {
        if (info->has_relation) {
            //toDo сравнение с заданным шаблоном, удаление указанной связи, подсчёт кол-ва удалённых связей
            return build_xml_create_or_delete_response("delete", "relation", number);
        } else {
            //toDo сравнение с заданным шаблоном, удаление указанных узлов, подсчёт кол-ва удалённых узлов
            return build_xml_create_or_delete_response("delete", "node", number);
        }
    }
    if (strcmp(command, "set") == 0) {
        //toDo сравнение с заданным шаблоном, изменение changed, подсчёт кол-ва изменений
        linked_list *node_ptr = init_list();
        number = match(info, data, node_ptr, NULL);
        if (info->changed_labels->size > 0) {
            set_new_labels(data, node_ptr, info->changed_labels);
            return build_xml_set_or_remove_response("set", "labels", info->changed_labels, number);
        } else if (info->changed_props->size > 0) {
            return build_xml_set_or_remove_response("set", "props", info->changed_props, number);
        }
        return NULL;
    }
    if (strcmp(command, "remove") == 0) {
        //toDo сравнение с заданным шаблоном, удаление changed, подсчёт кол-ва изменений
        linked_list *node_ptr = init_list();
        match(info, data, node_ptr, NULL);
        if (info->changed_labels->size > 0) {
            number = remove_labels(data, node_ptr, info->changed_labels);
            return build_xml_set_or_remove_response("remove", "labels", info->changed_labels, number);
        } else if (info->changed_props->size > 0) {
            return build_xml_set_or_remove_response("remove", "props", info->changed_props, number);
        }
        return NULL;
    }
    return NULL;
}

void close_server(server_info *info) {
    close(info->server_fd);
}

static server_info *create_server_info(uint16_t port) {
    server_info *server = malloc(sizeof(server_info));
    server->port = port;
    return server;
}
