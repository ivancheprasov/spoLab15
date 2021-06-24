#include <string.h>
#include "relation.h"
#include "cells.h"
#include "datafile.h"

cell_ptr *create_relation_cell(datafile *data, cell_ptr *string_cell, cell_ptr *first_node_cell, cell_ptr *second_node_cell) {
    if (data->ctrl_block->fragmented_relation_block == -1) {
        allocate_new_block(data, RELATION);
    }
    relation_cell new_cell = {0};
    memcpy(&new_cell.name, string_cell, sizeof(cell_ptr));
    memcpy(&new_cell.node_b, second_node_cell, sizeof(cell_ptr));

    block read_node = {0};
    fill_block(data, first_node_cell->block_num, &read_node);
    if (read_node.metadata.type == CONTROL) {
        memcpy(&new_cell.prev, &((control_block *) &read_node)->nodes[first_node_cell->offset].last_relation, sizeof(cell_ptr));
    } else {
        memcpy(&new_cell.prev, &((node_block *) &read_node)->nodes[first_node_cell->offset].last_relation, sizeof(cell_ptr));
    }

    cell_ptr *ptr = malloc(sizeof(cell_ptr));
    int32_t block_number = data->ctrl_block->fragmented_relation_block;
    ptr->block_num = block_number;
    ptr->offset = data->ctrl_block->empty_relation_number;

    relation_block read_relation;
    fill_block(data, block_number, &read_relation);

    relation_cell old_cell = {0};
    memcpy(&old_cell, &read_relation.relations[ptr->offset], sizeof(relation_cell));
    memcpy(&read_relation.relations[ptr->offset], &new_cell, sizeof(relation_cell));

    int16_t new_relation_offset;
    if (old_cell.prev.block_num == 0 && old_cell.prev.offset == 0) {
        new_relation_offset = (int16_t) (data->ctrl_block->empty_relation_number + 1);
        if (new_relation_offset > RELATIONS_IN_BLOCK) {
            new_relation_offset = 0;
            data->ctrl_block->fragmented_relation_block = -1;
        }
        data->ctrl_block->empty_relation_number = new_relation_offset;
    } else {
        data->ctrl_block->fragmented_relation_block = old_cell.prev.block_num;
        data->ctrl_block->empty_relation_number = old_cell.prev.offset;
    }
    update_data_block(data, block_number, &read_relation);
    update_control_block(data);
    return ptr;
}