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

typedef struct {
    cell_ptr *ptr;
    cell_ptr *next;
} attribute_info;

datafile *init_data(char *file_path);

void *get_block(datafile *data, int32_t block_number);

cell_ptr *create_node_cell(datafile *data);

cell_ptr *create_label_cell(datafile *data, cell_ptr *string_cell, cell_ptr *node_cell);

cell_ptr *create_attribute_cell(datafile *data, cell_ptr *key_cell, cell_ptr *value_cell, cell_ptr *node_cell);

void fill_block(datafile *data, int32_t block_number, void *buffer);

cell_ptr *create_string_cell(datafile *data, char *string);

void update_node_labels(datafile *data, cell_ptr *node_ptr, cell_ptr *label_ptr);

void update_node_attributes(datafile *data, cell_ptr *node_ptr, cell_ptr *attribute_ptr);

long match(query_info *info, datafile *data, linked_list *node_ptr, linked_list *nodes);

void set_new_labels(datafile *data, linked_list *node_cells, linked_list *changed_labels);

void set_new_attributes(datafile *data, linked_list *node_cells, linked_list *changed_props);

long remove_labels(datafile *data, linked_list *node_cells, linked_list *changed_labels);

long remove_attributes(datafile *data, linked_list *node_cells, linked_list *changed_props);

void delete_nodes(datafile *data, linked_list *node_cells);

attribute_info *find_node_attribute(datafile *data, cell_ptr *node_ptr, char *matcher_key);

static void update_control_block(datafile *data);

static bool match_labels(linked_list *matcher_labels, datafile *data, label_cell last_label, linked_list *node_labels);

static bool match_attributes(linked_list *matcher_attributes, datafile *data, attribute_cell last_attribute, linked_list *node_attributes);

static void update_data_block(datafile *data, int32_t block_number, void *block);

static void allocate_new_block(datafile *data, TYPE type); //get_control_block->first_empty_block; control_block->fragmented...=new

#endif //SPOLAB15_DATAFILE_H

