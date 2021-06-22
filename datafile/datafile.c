#include <unistd.h>
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include "datafile.h"

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
        data->ctrl_block = ctrl_block;
        data->file = data_file;
        sleep(1);
        update_control_block(data);
    }
    return data;
}

cell_ptr *create_node_cell(datafile *data) {
    if (data->ctrl_block->fragmented_node_block == -1) {
        allocate_new_block(data, NODE);
    }
    node_cell new_cell = {0};
    cell_ptr *ptr = malloc(sizeof(cell_ptr));

    ptr->block_num = data->ctrl_block->fragmented_node_block;
    ptr->offset = data->ctrl_block->empty_node_number;

    int32_t block_number = data->ctrl_block->fragmented_node_block;
    block read_node;
    fill_block(data, data->ctrl_block->fragmented_node_block, &read_node);
    node_cell old_cell;

    if(read_node.metadata.type == CONTROL) {
        old_cell = ((control_block *)&read_node)->nodes[data->ctrl_block->empty_node_number];
        memcpy(&((control_block *)&read_node)->nodes[data->ctrl_block->empty_node_number], &new_cell, sizeof(node_cell));
    } else {
        old_cell = ((node_block *)&read_node)->nodes[data->ctrl_block->empty_node_number];
        memcpy(&((node_block *)&read_node)->nodes[data->ctrl_block->empty_node_number], &new_cell, sizeof(node_cell));
    }

    int16_t new_node_offset;
    if (old_cell.last_label.block_num == 0 && old_cell.last_label.offset == 0) {
        new_node_offset = (int16_t) (data->ctrl_block->empty_node_number + 1);
        if (new_node_offset > NODES_IN_CONTROL_BLOCK && ptr->block_num == 0 || new_node_offset > NODES_IN_BLOCK && ptr->block_num != 0) {
            new_node_offset = 0;
            data->ctrl_block->fragmented_node_block = -1;
        }
        data->ctrl_block->empty_node_number = new_node_offset;
    } else {
        data->ctrl_block->fragmented_node_block = old_cell.last_label.block_num;
        data->ctrl_block->empty_node_number = old_cell.last_label.offset;
    }
    update_data_block(data, block_number, &read);
    update_control_block(data);
    return ptr;
}

cell_ptr *create_label_cell(datafile *data, cell_ptr *string_cell, cell_ptr *node_cell) {
    if (data->ctrl_block->fragmented_label_block == -1) {
        allocate_new_block(data, LABEL);
    }
    label_cell new_cell = {0};
    memcpy(&new_cell.name, string_cell, sizeof(cell_ptr));

    block read_node = {0};
    fill_block(data, node_cell->block_num, &read_node);
    if(read_node.metadata.type != NODE) {
        memcpy(&new_cell.prev, &((control_block *) &read_node)->nodes[node_cell->offset].last_label, sizeof(cell_ptr));
    } else {
        memcpy(&new_cell.prev, &((node_block *) &read_node)->nodes[node_cell->offset].last_label, sizeof(cell_ptr));
    }

    cell_ptr *ptr = malloc(sizeof(cell_ptr));
    ptr->block_num = data->ctrl_block->fragmented_label_block;
    ptr->offset = data->ctrl_block->empty_label_number;
    int32_t block_number = data->ctrl_block->fragmented_label_block;
    label_block read;
    fill_block(data, data->ctrl_block->fragmented_label_block, &read);

    label_cell old_cell = {0};
    memcpy(&old_cell, &read.labels[data->ctrl_block->empty_label_number], sizeof(label_cell));
    memcpy(&read.labels[data->ctrl_block->empty_label_number], &new_cell, sizeof(label_cell));

    int16_t new_label_offset;
    if (old_cell.prev.block_num == 0 && old_cell.prev.offset == 0) {
        new_label_offset = (int16_t) (data->ctrl_block->empty_label_number + 1);
        if (new_label_offset > LABELS_IN_BLOCK) {
            new_label_offset = 0;
            data->ctrl_block->fragmented_label_block = -1;
        }
        data->ctrl_block->empty_label_number = new_label_offset;
    } else {
        data->ctrl_block->fragmented_label_block = old_cell.prev.block_num;
        data->ctrl_block->empty_label_number = old_cell.prev.offset;
    }
    update_data_block(data, block_number, &read);
    update_control_block(data);
    return ptr;
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
            //todo write pointer to prev fragment
            ptr->block_num = data->ctrl_block->fragmented_string_block;
            ptr->offset = data->ctrl_block->empty_string_offset;
            data->ctrl_block->empty_string_offset += str_length + 2;
            memcpy(read.data + ptr->offset, &str_length, 2);
            strcpy(read.data + ptr->offset + 2, string);
        } else {
            //todo create new block
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
            //todo create new block
        } else {
            //todo write to block by offset ptr
        }
    }
    update_data_block(data, ptr->block_num, &read);
    update_control_block(data);
    return ptr;
}

void update_node_labels(datafile *data, cell_ptr *node_ptr, cell_ptr *label_ptr) {
    node_cell node = {0};
    block read_node;
    fill_block(data, node_ptr->block_num, &read_node);
    if(read_node.metadata.type == CONTROL) {
        memcpy(&node, &((control_block *) &read_node)->nodes[node_ptr->offset], sizeof(node_cell));
        memcpy(&node.last_label, label_ptr, sizeof (cell_ptr));
        memcpy(&((control_block *)&read_node)->nodes[node_ptr->offset], &node, sizeof (node_cell));
    } else {
        memcpy(&node, &((node_block *) &read_node)->nodes[node_ptr->offset], sizeof(node_cell));
        memcpy(&node.last_label, label_ptr, sizeof (cell_ptr));
        memcpy(&((node_block *)&read_node)->nodes[node_ptr->offset], &node, sizeof (node_cell));
    }
    update_data_block(data, node_ptr->block_num, &read_node);
    fill_block(data, 0, data->ctrl_block);
}

static void allocate_new_block(datafile *data, TYPE type) {
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
            if(block_number == 1){
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

static void update_control_block(datafile *data) {
    update_data_block(data, 0, data->ctrl_block);
}

static void update_data_block(datafile *data, int32_t block_number, void *block) {
    fseek(data->file, block_number * BLOCK_SIZE, SEEK_SET);
    fwrite(block, BLOCK_SIZE, 1, data->file);
}
