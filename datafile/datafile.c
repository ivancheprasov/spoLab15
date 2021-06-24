#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include "datafile.h"
#include "label.h"
#include "attribute.h"

datafile *init_data(char *file_path) {
    datafile *data = malloc(sizeof(datafile));
    FILE *data_file;
    if (access(file_path, F_OK) == 0) {
        data_file = fopen(file_path, "r+b");
        data->file = data_file;
        data->ctrl_block = malloc(sizeof(control_block));
        fill_block(data, 0, data->ctrl_block);
    } else {
        data_file = fopen(file_path, "wb+");
        control_block *ctrl_block = malloc(sizeof(control_block));
        bzero(ctrl_block, sizeof(control_block));
        ctrl_block->metadata.type = CONTROL;
        ctrl_block->empty_block = 1;
        ctrl_block->fragmented_label_block = -1;
        ctrl_block->fragmented_relation_block = -1;
        ctrl_block->fragmented_string_block = -1;
        ctrl_block->fragmented_attribute_block = -1;
        ctrl_block->empty_node_number = 0;
        data->ctrl_block = ctrl_block;
        data->file = data_file;
        sleep(1);
        update_control_block(data);
    }
    return data;
}

void fill_block(datafile *data, int32_t block_number, void *buffer) {
    fseek(data->file, block_number * BLOCK_SIZE, SEEK_SET);
    fread(buffer, BLOCK_SIZE, 1, data->file);
}

cell_ptr *create_string_cell(datafile *data, char *string) {
    if (data->ctrl_block->fragmented_string_block == -1) {
        allocate_new_block(data, STRING);
    }
    int16_t str_length = (int16_t) strlen(string);
    int32_t block_number = data->ctrl_block->fragmented_string_block;
    str_block read;
    fill_block(data, data->ctrl_block->fragmented_string_block, &read);
    int16_t empty_fragment_size = 0;

    cell_ptr *ptr = malloc(sizeof(cell_ptr));

    memcpy(&empty_fragment_size, read.data + data->ctrl_block->empty_string_offset, sizeof(empty_fragment_size));
    if (empty_fragment_size == 0) {
        empty_fragment_size = (int16_t) (1020 - data->ctrl_block->empty_string_offset);
        if (empty_fragment_size > str_length) {
            ptr->block_num = data->ctrl_block->fragmented_string_block;
            ptr->offset = data->ctrl_block->empty_string_offset;
            data->ctrl_block->empty_string_offset += str_length + 2;
            memcpy(read.data + ptr->offset, &str_length, 2);
            strcpy(read.data + ptr->offset + 2, string);
        } else {
            int32_t new_block_number = data->ctrl_block->empty_block;
            allocate_new_block(data, STRING);
            data->ctrl_block->fragmented_string_block = new_block_number;
            data->ctrl_block->empty_string_offset = str_length + 2;
            ptr->offset = 0;
            ptr->block_num = new_block_number;
            fill_block(data, new_block_number, &read);
            memcpy(read.data, &str_length, 2);
            strcpy(read.data + 2, string);
        }
    } else {
        cell_ptr offset_ptr = {0};
        do {
            memcpy(&offset_ptr, read.data + data->ctrl_block->empty_string_offset + sizeof(empty_fragment_size),
                   sizeof(cell_ptr));
            if (offset_ptr.block_num != block_number) {
                fill_block(data, offset_ptr.block_num, &read);
            }
            memcpy(&empty_fragment_size, read.data + offset_ptr.offset, sizeof(empty_fragment_size));
        } while (empty_fragment_size < str_length || offset_ptr.block_num != 0);
        if (offset_ptr.block_num == 0) {
            int32_t new_block_number = data->ctrl_block->empty_block;
            allocate_new_block(data, STRING);
            data->ctrl_block->fragmented_string_block = new_block_number;
            data->ctrl_block->empty_string_offset = str_length + 2;
            ptr->offset = 0;
            ptr->block_num = new_block_number;
            fill_block(data, new_block_number, &read);
            memcpy(read.data, &str_length, 2);
            strcpy(read.data + 2, string);
        }
    }
    update_data_block(data, ptr->block_num, &read);
    update_control_block(data);
    return ptr;
}

long match(query_info *info, datafile *data, linked_list *node_ptr, linked_list *nodes) {
    int32_t node_block_num = 0;
    long number = 0;
    do {
        block read_node = {0};
        fill_block(data, node_block_num, &read_node);
        int16_t nodes_in_block = read_node.metadata.type == CONTROL ? NODES_IN_CONTROL_BLOCK : NODES_IN_BLOCK;
        for (int16_t offset = 0; offset < nodes_in_block; ++offset) {
            node_cell node = {0};
            if (read_node.metadata.type == CONTROL) {
                memcpy(&node, &((control_block *) &read_node)->nodes[offset], sizeof(node_cell));
            } else {
                memcpy(&node, &((node_block *) &read_node)->nodes[offset], sizeof(node_cell));
            }
            if (node.is_empty == 0) {
                if (node.last_label.offset == 0 && node.last_label.block_num == 0) {
                    break;
                } else {
                    continue;
                }
            }
            label_block read_label = {0};
            attribute_block read_attribute = {0};
            fill_block(data, node.last_label.block_num, &read_label);
            fill_block(data, node.last_attribute.block_num, &read_attribute);
            label_cell label = {0};
            attribute_cell attribute = {0};
            if (read_label.metadata.type != CONTROL) {
                memcpy(&label, &read_label.labels[node.last_label.offset], sizeof(label_cell));
            }
            if (read_attribute.metadata.type != CONTROL) {
                memcpy(&attribute, &read_attribute.attributes[node.last_attribute.offset], sizeof(attribute_cell));
            }
            linked_list *node_labels = init_list();
            linked_list *node_props = init_list();
            if (nodes == NULL) {
                if (!match_labels(info->labels, data, label, NULL)) {
                    free_list(node_labels);
                    continue;
                }
                if (!match_attributes(info->props, data, attribute, NULL)) {
                    free_list(node_props);
                    continue;
                }
            } else {
                if (!match_labels(info->labels, data, label, node_labels)) {
                    free_list(node_labels);
                    continue;
                }
                if (!match_attributes(info->props, data, attribute, node_props)) {
                    free_list(node_props);
                    continue;
                }
            }
            cell_ptr *ptr = malloc(sizeof(cell_ptr));
            ptr->block_num = node_block_num;
            ptr->offset = offset;
            add_last(node_ptr, ptr);
            if (nodes != NULL) {
                match_result *match = malloc(sizeof(match_result));
                match->labels = node_labels;
                match->props = node_props;
                add_last(nodes, match);
            }
            number += 1;
        }
        node_block_num = read_node.metadata.type == CONTROL ?
                         ((control_block *) &read_node)->next_node_block
                                                            :
                         ((node_block *) &read_node)->next_block;
    } while (node_block_num != 0);
    return number;
}

void allocate_new_block(datafile *data, TYPE type) {
    fseek(data->file, data->ctrl_block->empty_block * BLOCK_SIZE, SEEK_SET);
    block new_block = {0};
    new_block.metadata.type = type;
    int32_t block_number = data->ctrl_block->empty_block;
    update_data_block(data, block_number, &new_block);
    switch (type) {
        case CONTROL:
            break;
        case NODE:
            data->ctrl_block->fragmented_node_block = block_number;
            if (block_number == 1) {
                data->ctrl_block->next_node_block = block_number;
            } else {
                node_block block = {0};
                fill_block(data, data->ctrl_block->next_node_block, &block);
                block.next_block = block_number;
                update_data_block(data, data->ctrl_block->next_node_block, &block);
            }
            break;
        case LABEL:
            data->ctrl_block->fragmented_label_block = block_number;
            break;
        case ATTRIBUTE:
            data->ctrl_block->fragmented_attribute_block = block_number;
            break;
        case RELATION:
            data->ctrl_block->fragmented_relation_block = block_number;
            break;
        case STRING:
            data->ctrl_block->fragmented_string_block = block_number;
            break;
        default:
            break;
    }
    data->ctrl_block->empty_block = block_number + 1;
    update_control_block(data);
}

void update_control_block(datafile *data) {
    update_data_block(data, 0, data->ctrl_block);
}

void update_data_block(datafile *data, int32_t block_number, void *block) {
    fseek(data->file, block_number * BLOCK_SIZE, SEEK_SET);
    fwrite(block, BLOCK_SIZE, 1, data->file);
}