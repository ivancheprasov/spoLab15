#include <bits/types/FILE.h>
#include <stdint.h>
#include <stdbool.h>
#include "blocks.h"
#include "../utils/linked_list.h"
#include "../utils/message.h"

#ifndef SPOLAB15_DATAFILE_H
#define SPOLAB15_DATAFILE_H

typedef struct {
    FILE* file;
    control_block *ctrl_block;
} datafile;

datafile *init_data(char *file_path);

void fill_block(datafile *data, int32_t block_number, void *buffer);

cell_ptr *create_string_cell(datafile *data, char *string);

long match(query_info *info, datafile *data, linked_list *node_ptr, linked_list *nodes, bool is_node_a);

void update_control_block(datafile *data);

void update_data_block(datafile *data, int32_t block_number, void *block);

void allocate_new_block(datafile *data, TYPE type);

#endif //SPOLAB15_DATAFILE_H

