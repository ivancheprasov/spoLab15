#include <unistd.h>
#include <stdio.h>
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
        ctrl_block->empty_node_number = 0;
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
    new_cell.is_empty = 1;
    cell_ptr *ptr = malloc(sizeof(cell_ptr));

    ptr->block_num = data->ctrl_block->fragmented_node_block;
    ptr->offset = data->ctrl_block->empty_node_number;

    int32_t block_number = data->ctrl_block->fragmented_node_block;
    block read_node;
    fill_block(data, data->ctrl_block->fragmented_node_block, &read_node);
    node_cell old_cell;

    if (read_node.metadata.type == CONTROL) {
        old_cell = ((control_block *) &read_node)->nodes[data->ctrl_block->empty_node_number];
        memcpy(&((control_block *) &read_node)->nodes[data->ctrl_block->empty_node_number], &new_cell,
               sizeof(node_cell));
        update_data_block(data, block_number, &read_node);
        fill_block(data, 0, data->ctrl_block);
    } else {
        old_cell = ((node_block *) &read_node)->nodes[data->ctrl_block->empty_node_number];
        memcpy(&((node_block *) &read_node)->nodes[data->ctrl_block->empty_node_number], &new_cell, sizeof(node_cell));
        update_data_block(data, block_number, &read_node);
    }

    int16_t new_node_offset;
    if (old_cell.last_label.block_num == 0 && old_cell.last_label.offset == 0) {
        new_node_offset = (int16_t) (data->ctrl_block->empty_node_number + 1);
        if (new_node_offset > NODES_IN_CONTROL_BLOCK && ptr->block_num == 0 ||
            new_node_offset > NODES_IN_BLOCK && ptr->block_num != 0) {
            new_node_offset = 0;
            data->ctrl_block->fragmented_node_block = -1;
        }
        data->ctrl_block->empty_node_number = new_node_offset;
    } else {
        data->ctrl_block->fragmented_node_block = old_cell.last_label.block_num;
        data->ctrl_block->empty_node_number = old_cell.last_label.offset;
    }
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
    if (read_node.metadata.type == CONTROL) {
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

cell_ptr *create_attribute_cell(datafile *data, cell_ptr *key_cell, cell_ptr *value_cell, cell_ptr *node_cell) {
    if (data->ctrl_block->fragmented_attribute_block == -1) {
        allocate_new_block(data, ATTRIBUTE);
    }
    attribute_cell new_cell = {0};
    memcpy(&new_cell.key, key_cell, sizeof(cell_ptr));
    memcpy(&new_cell.value, value_cell, sizeof(cell_ptr));

    block read_node = {0};
    fill_block(data, node_cell->block_num, &read_node);
    if (read_node.metadata.type == CONTROL) {
        memcpy(&new_cell.prev, &((control_block *) &read_node)->nodes[node_cell->offset].last_attribute,
               sizeof(cell_ptr));
    } else {
        memcpy(&new_cell.prev, &((node_block *) &read_node)->nodes[node_cell->offset].last_attribute, sizeof(cell_ptr));
    }

    cell_ptr *ptr = malloc(sizeof(cell_ptr));
    ptr->block_num = data->ctrl_block->fragmented_attribute_block;
    ptr->offset = data->ctrl_block->empty_attribute_number;
    int32_t block_number = data->ctrl_block->fragmented_attribute_block;
    attribute_block read_attribute;
    fill_block(data, data->ctrl_block->fragmented_attribute_block, &read_attribute);

    attribute_cell old_cell = {0};
    memcpy(&old_cell, &read_attribute.attributes[data->ctrl_block->empty_attribute_number], sizeof(attribute_cell));
    memcpy(&read_attribute.attributes[data->ctrl_block->empty_attribute_number], &new_cell, sizeof(attribute_cell));

    int16_t new_attribute_offset;
    if (old_cell.prev.block_num == 0 && old_cell.prev.offset == 0) {
        new_attribute_offset = (int16_t) (data->ctrl_block->empty_attribute_number + 1);
        if (new_attribute_offset > ATTRIBUTES_IN_BLOCK) {
            new_attribute_offset = 0;
            data->ctrl_block->fragmented_attribute_block = -1;
        }
        data->ctrl_block->empty_attribute_number = new_attribute_offset;
    } else {
        data->ctrl_block->fragmented_attribute_block = old_cell.prev.block_num;
        data->ctrl_block->empty_attribute_number = old_cell.prev.offset;
    }
    update_data_block(data, block_number, &read_attribute);
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

void update_node_labels(datafile *data, cell_ptr *node_ptr, cell_ptr *label_ptr) {
    node_cell node = {0};
    block read_node;
    fill_block(data, node_ptr->block_num, &read_node);
    if (read_node.metadata.type == CONTROL) {
        memcpy(&node, &((control_block *) &read_node)->nodes[node_ptr->offset], sizeof(node_cell));
        memcpy(&node.last_label, label_ptr, sizeof(cell_ptr));
        memcpy(&((control_block *) &read_node)->nodes[node_ptr->offset], &node, sizeof(node_cell));
    } else {
        memcpy(&node, &((node_block *) &read_node)->nodes[node_ptr->offset], sizeof(node_cell));
        memcpy(&node.last_label, label_ptr, sizeof(cell_ptr));
        memcpy(&((node_block *) &read_node)->nodes[node_ptr->offset], &node, sizeof(node_cell));
    }
    update_data_block(data, node_ptr->block_num, &read_node);
    fill_block(data, 0, data->ctrl_block);
}

void update_node_attributes(datafile *data, cell_ptr *node_ptr, cell_ptr *attribute_ptr) {
    node_cell node = {0};
    block read_node;
    fill_block(data, node_ptr->block_num, &read_node);
    if (read_node.metadata.type == CONTROL) {
        memcpy(&node, &((control_block *) &read_node)->nodes[node_ptr->offset], sizeof(node_cell));
        memcpy(&node.last_attribute, attribute_ptr, sizeof(cell_ptr));
        memcpy(&((control_block *) &read_node)->nodes[node_ptr->offset], &node, sizeof(node_cell));
    } else {
        memcpy(&node, &((node_block *) &read_node)->nodes[node_ptr->offset], sizeof(node_cell));
        memcpy(&node.last_attribute, attribute_ptr, sizeof(cell_ptr));
        memcpy(&((node_block *) &read_node)->nodes[node_ptr->offset], &node, sizeof(node_cell));
    }
    update_data_block(data, node_ptr->block_num, &read_node);
    fill_block(data, 0, data->ctrl_block);
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

static bool match_labels(linked_list *matcher_labels, datafile *data, label_cell last_label, linked_list *node_labels) {
    label_cell label = last_label;
    cell_ptr prev;
    linked_list *labels = init_list();
    for (node *current_matcher_label = matcher_labels->first; current_matcher_label; current_matcher_label = current_matcher_label->next) {
        add_last(labels, current_matcher_label->value);
    }
    if (label.name.block_num == 0) return labels->size == 0;
    do {
        cell_ptr string_ptr = label.name;
        str_block read_string = {0};
        fill_block(data, string_ptr.block_num, &read_string);
        int16_t size = 0;
        memcpy(&size, &read_string.data[string_ptr.offset], sizeof(int16_t));
        char *label_name = malloc(size + 1);
        bzero(label_name, strlen(label_name) + 1);
        strcpy(label_name, &read_string.data[string_ptr.offset + 2]);
        label_name[size] = '\0';
        if (node_labels != NULL) add_last(node_labels, label_name);
        remove_element(by_value, labels, label_name, NULL);
        label_block read_labels = {0};
        prev = label.prev;
        fill_block(data, prev.block_num, &read_labels);
        memcpy(&label, &read_labels.labels[prev.offset], sizeof(label_cell));
    } while (!(prev.block_num == 0 && prev.offset == 0));
    return labels->size == 0;
}

static bool match_attributes(linked_list *matcher_attributes, datafile *data, attribute_cell last_attribute,
                             linked_list *node_attributes) {
    attribute_cell attribute = last_attribute;
    cell_ptr prev;
    linked_list *attributes = init_list();
    for (node *current_matcher_attribute = matcher_attributes->first; current_matcher_attribute; current_matcher_attribute = current_matcher_attribute->next) {
        property *prop = malloc(sizeof(property));
        bzero(prop, sizeof(property));
        strcpy(prop->key, ((property *) current_matcher_attribute->value)->key);
        strcpy(prop->value, ((property *) current_matcher_attribute->value)->value);
        add_last(attributes, prop);
    }
    if (attribute.key.block_num == 0 || attribute.value.block_num == 0) return attributes->size == 0;
    do {
        cell_ptr key_ptr = attribute.key;
        cell_ptr value_ptr = attribute.value;
        str_block read_key = {0};
        str_block read_value = {0};
        fill_block(data, key_ptr.block_num, &read_key);
        fill_block(data, value_ptr.block_num, &read_value);
        int16_t key_size = 0;
        int16_t value_size = 0;
        memcpy(&key_size, &read_key.data[key_ptr.offset], sizeof(int16_t));
        memcpy(&value_size, &read_value.data[value_ptr.offset], sizeof(int16_t));
        char *attr_key = malloc(key_size + 1);
        char *attr_value = malloc(value_size + 1);
        bzero(attr_key, strlen(attr_key) + 1);
        bzero(attr_value, strlen(attr_value) + 1);
        strcpy(attr_key, &read_key.data[key_ptr.offset + 2]);
        strcpy(attr_value, &read_value.data[value_ptr.offset + 2]);
        attr_key[key_size] = '\0';
        attr_value[value_size] = '\0';
        if (node_attributes != NULL) {
            property *prop = malloc(sizeof(property));
            bzero(prop, sizeof(property));
            strcpy(prop->key, attr_key);
            strcpy(prop->value, attr_value);
            add_last(node_attributes, prop);
        }
        remove_element(by_property_values, attributes, attr_key, attr_value);
        attribute_block read_attributes = {0};
        prev = attribute.prev;
        fill_block(data, prev.block_num, &read_attributes);
        memcpy(&attribute, &read_attributes.attributes[prev.offset], sizeof(attribute_cell));
    } while (!(prev.block_num == 0 && prev.offset == 0));
    return attributes->size == 0;
}

void set_new_labels(datafile *data, linked_list *node_cells, linked_list *changed_labels) {
    node *label;
    for (node *current_node = node_cells->first; current_node; current_node = current_node->next) {
        cell_ptr *node_ptr = current_node->value;
        for (label = changed_labels->first; label; label = label->next) {
            cell_ptr *string_ptr = create_string_cell(data, label->value);
            cell_ptr *label_ptr = create_label_cell(data, string_ptr, node_ptr);
            update_node_labels(data, node_ptr, label_ptr);
        }
    }
}

void set_new_attributes(datafile *data, linked_list *node_cells, linked_list *changed_props) {
    node *prop;
    for (node *current_node = node_cells->first; current_node; current_node = current_node->next) {
        cell_ptr *node_ptr = current_node->value;
        for (prop = changed_props->first; prop; prop = prop->next) {
            attribute_info *found = find_node_attribute(data, node_ptr, ((property *) prop->value)->key);
            if (found == NULL) {
                cell_ptr *key_ptr = create_string_cell(data, ((property *) prop->value)->key);
                cell_ptr *value_ptr = create_string_cell(data, ((property *) prop->value)->value);
                cell_ptr *attr_ptr = create_attribute_cell(data, key_ptr, value_ptr, node_ptr);
                update_node_attributes(data, node_ptr, attr_ptr);
            } else {
                attribute_block read_attribute;
                fill_block(data, found->ptr->block_num, &read_attribute);
                attribute_cell attribute = {0};
                memcpy(&attribute, &read_attribute.attributes[found->ptr->offset], sizeof(attribute_cell));
                cell_ptr *value_ptr = create_string_cell(data, ((property *) prop->value)->value);
                memcpy(&attribute.value, value_ptr, sizeof(cell_ptr));
                memcpy(&read_attribute.attributes[found->ptr->offset], &attribute, sizeof(attribute_cell));
                update_data_block(data, found->ptr->block_num, &read_attribute);
            }
        }
    }
}

attribute_info *find_node_attribute(datafile *data, cell_ptr *node_ptr, char *matcher_key) {
    attribute_info *result = NULL;
    block read_node = {0};
    fill_block(data, node_ptr->block_num, &read_node);
    node_cell node = {0};
    if (read_node.metadata.type == CONTROL) {
        memcpy(&node, &((control_block *) &read_node)->nodes[node_ptr->offset], sizeof(node_cell));
    } else {
        memcpy(&node, &((node_block *) &read_node)->nodes[node_ptr->offset], sizeof(node_cell));
    }
    cell_ptr attribute_ptr = node.last_attribute;
    if (attribute_ptr.block_num == 0) return NULL;
    int32_t block_number = attribute_ptr.block_num;
    int16_t offset = attribute_ptr.offset;
    cell_ptr *next = malloc(sizeof(cell_ptr));
    memcpy(next, node_ptr, sizeof(cell_ptr));
    do {
        attribute_block read_attribute;
        fill_block(data, block_number, &read_attribute);
        attribute_cell *attribute = malloc(sizeof(attribute_cell));
        memcpy(attribute, &read_attribute.attributes[offset], sizeof(attribute_cell));
        cell_ptr key_ptr = attribute->key;
        str_block read_string = {0};
        fill_block(data, key_ptr.block_num, &read_string);
        int16_t size = 0;
        memcpy(&size, &read_string.data[key_ptr.offset], sizeof(int16_t));
        char *key = malloc(size + 1);
        bzero(key, strlen(key) + 1);
        strcpy(key, &read_string.data[key_ptr.offset + 2]);
        key[size] = '\0';
        if (strcmp(key, matcher_key) == 0) {
            cell_ptr *ptr = malloc(sizeof(cell_ptr));
            memcpy(&ptr->block_num, &block_number, sizeof(int32_t));
            memcpy(&ptr->offset, &offset, sizeof(int16_t));
            attribute_info *info = malloc(sizeof(attribute_info));
            info->ptr = ptr;
            info->next = next;
            return info;
        } else {
            memcpy(&next->block_num, &block_number, sizeof(int32_t));
            memcpy(&next->offset, &offset, sizeof(int16_t));
            block_number = attribute->prev.block_num;
            offset = attribute->prev.offset;
        }
    } while (block_number != 0);
    return result;
}

void delete_nodes(datafile *data, linked_list *node_cells) {
    remove_labels(data, node_cells, NULL);
    remove_attributes(data, node_cells, NULL);
    for (node *current_node = node_cells->first; current_node; current_node = current_node->next) {
        block read_node = {0};
        cell_ptr *node_ptr = current_node->value;
        fill_block(data, node_ptr->block_num, &read_node);
        node_cell node = {0};
        if (read_node.metadata.type == CONTROL) {
            memcpy(&node, &((control_block *) &read_node)->nodes[node_ptr->offset], sizeof(node_cell));
            node.is_empty = 0;
            memcpy(&node.last_label.block_num, &data->ctrl_block->fragmented_node_block, sizeof(int32_t));
            memcpy(&node.last_label.offset, &data->ctrl_block->empty_node_number, sizeof(int16_t));
            memcpy(&((control_block *) &read_node)->nodes[node_ptr->offset], &node, sizeof(node_cell));
        } else {
            memcpy(&node, &((node_block *) &read_node)->nodes[node_ptr->offset], sizeof(node_cell));
            node.is_empty = 0;
            memcpy(&node.last_label.block_num, &data->ctrl_block->fragmented_node_block, sizeof(int32_t));
            memcpy(&node.last_label.offset, &data->ctrl_block->empty_node_number, sizeof(int16_t));
            memcpy(&((node_block *) &read_node)->nodes[node_ptr->offset], &node, sizeof(node_cell));
        }
        update_data_block(data, node_ptr->block_num, &read_node);
        fill_block(data, 0, data->ctrl_block);
        memcpy(&data->ctrl_block->fragmented_node_block, &node_ptr->block_num, sizeof(int32_t));
        memcpy(&data->ctrl_block->empty_node_number, &node_ptr->offset, sizeof(int16_t));
        update_control_block(data);
    }
}

long remove_labels(datafile *data, linked_list *node_cells, linked_list *changed_labels) {
    long number = 0;
    node *query_label;
    label_block read_label;
    for (node *current_node = node_cells->first; current_node; current_node = current_node->next) {
        bool has_changed = false;
        block read_node = {0};
        cell_ptr *node_ptr = current_node->value;
        fill_block(data, node_ptr->block_num, &read_node);
        node_cell node = {0};
        if (read_node.metadata.type == CONTROL) {
            memcpy(&node, &((control_block *) &read_node)->nodes[node_ptr->offset], sizeof(node_cell));
        } else {
            memcpy(&node, &((node_block *) &read_node)->nodes[node_ptr->offset], sizeof(node_cell));
        }
        linked_list *labels = init_list();
        if (changed_labels != NULL) {
            for (query_label = changed_labels->first; query_label; query_label = query_label->next) {
                add_last(labels, query_label->value);
            }
        }
        int32_t block_number = node.last_label.block_num;
        int16_t offset = node.last_label.offset;
        cell_ptr prev = {0};
        memcpy(&prev, node_ptr, sizeof(cell_ptr));
        do {
            bzero(&read_label, BLOCK_SIZE);
            fill_block(data, block_number, &read_label);
            if (read_label.metadata.type == CONTROL) break;
            cell_ptr current = {0};
            current.block_num = block_number;
            current.offset = offset;
            label_cell old_cell = {0};
            label_cell new_cell = {0};
            memcpy(&old_cell, &read_label.labels[offset], sizeof(label_cell));
            cell_ptr string_ptr = old_cell.name;
            str_block read_string = {0};
            fill_block(data, string_ptr.block_num, &read_string);
            int16_t size = 0;
            memcpy(&size, &read_string.data[string_ptr.offset], sizeof(int16_t));
            char *label_name = malloc(size + 1);
            bzero(label_name, strlen(label_name) + 1);
            strcpy(label_name, &read_string.data[string_ptr.offset + 2]);
            label_name[size] = '\0';
            if (changed_labels == NULL || find_element(by_value, labels, label_name, NULL) != NULL) {
                remove_element(by_value, labels, label_name, NULL);
                bzero(&new_cell, sizeof(label_cell));
                block updated = {0};
                fill_block(data, prev.block_num, &updated);
                if (updated.metadata.type == CONTROL) {
                    memcpy(&node.last_label, &old_cell.prev, sizeof(cell_ptr));
                    memcpy(&((control_block *) &updated)->nodes[node_ptr->offset], &node, sizeof(node_cell));
                    update_data_block(data, node_ptr->block_num, &updated);
                    fill_block(data, 0, data->ctrl_block);
                } else if (updated.metadata.type == NODE) {
                    memcpy(&node.last_label, &old_cell.prev, sizeof(cell_ptr));
                    memcpy(&((node_block *) &updated)->nodes[node_ptr->offset], &node, sizeof(node_cell));
                    update_data_block(data, node_ptr->block_num, &updated);
                } else {
                    label_cell prev_label = ((label_block *) &updated)->labels[prev.offset];
                    memcpy(&prev_label.prev, &old_cell.prev, sizeof(cell_ptr));
                    memcpy(&((label_block *) &updated)->labels[prev.offset], &prev_label, sizeof(label_cell));
                    update_data_block(data, prev.block_num, &updated);
                }
                bzero(&read_label, BLOCK_SIZE);
                fill_block(data, block_number, &read_label);
                memcpy(&new_cell.prev.block_num, &data->ctrl_block->fragmented_label_block, sizeof(int32_t));
                memcpy(&new_cell.prev.offset, &data->ctrl_block->empty_label_number, sizeof(int16_t));
                memcpy(&((label_block *) &read_label)->labels[offset], &new_cell, sizeof(label_cell));
                update_data_block(data, block_number, &read_label);
                memcpy(&data->ctrl_block->fragmented_label_block, &block_number, sizeof(int32_t));
                memcpy(&data->ctrl_block->empty_label_number, &offset, sizeof(int16_t));
                update_control_block(data);
                if (!has_changed) number++;
                has_changed = true;
            }
            memcpy(&prev, &current, sizeof(cell_ptr));
            block_number = old_cell.prev.block_num;
            offset = old_cell.prev.offset;
        } while (read_label.metadata.type != CONTROL);
    }
    return number;
}

long remove_attributes(datafile *data, linked_list *node_cells, linked_list *changed_props) {
    long number = 0;
    node *query_attr;
    attribute_block read_attr = {0};
    for (node *current_node = node_cells->first; current_node; current_node = current_node->next) {
        bool has_changed = false;
        block read_node = {0};
        cell_ptr *node_ptr = current_node->value;
        fill_block(data, node_ptr->block_num, &read_node);
        node_cell node = {0};
        if (read_node.metadata.type == CONTROL) {
            memcpy(&node, &((control_block *) &read_node)->nodes[node_ptr->offset], sizeof(node_cell));
        } else {
            memcpy(&node, &((node_block *) &read_node)->nodes[node_ptr->offset], sizeof(node_cell));
        }
        linked_list *attributes = init_list();
        if (changed_props != NULL) {
            for (query_attr = changed_props->first; query_attr; query_attr = query_attr->next) {
                property *prop = malloc(sizeof(property));
                bzero(prop, sizeof(property));
                strcpy(prop->key, ((property *) query_attr->value)->key);
                strcpy(prop->value, ((property *) query_attr->value)->value);
                add_last(attributes, prop);
            }
        }
        int32_t block_number = node.last_attribute.block_num;
        int16_t offset = node.last_attribute.offset;
        cell_ptr prev = {0};
        memcpy(&prev, node_ptr, sizeof(cell_ptr));
        do {
            bzero(&read_attr, BLOCK_SIZE);
            fill_block(data, block_number, &read_attr);
            if (read_attr.metadata.type == CONTROL) break;
            cell_ptr current = {0};
            current.block_num = block_number;
            current.offset = offset;
            attribute_cell old_cell = {0};
            attribute_cell new_cell = {0};
            memcpy(&old_cell, &read_attr.attributes[offset], sizeof(attribute_cell));
            cell_ptr key_ptr = old_cell.key;
            str_block read_key = {0};
            fill_block(data, key_ptr.block_num, &read_key);
            int16_t size = 0;
            memcpy(&size, &read_key.data[key_ptr.offset], sizeof(int16_t));
            char *key = malloc(size + 1);
            bzero(key, strlen(key) + 1);
            strcpy(key, &read_key.data[key_ptr.offset + 2]);
            key[size] = '\0';
            if (changed_props == NULL || find_element(by_key, attributes, key, NULL) != NULL) {
                remove_element(by_key, attributes, key, NULL);
                bzero(&new_cell, sizeof(attribute_cell));
                block updated = {0};
                fill_block(data, prev.block_num, &updated);
                if (updated.metadata.type == CONTROL) {
                    memcpy(&node.last_attribute, &old_cell.prev, sizeof(cell_ptr));
                    memcpy(&((control_block *) &updated)->nodes[node_ptr->offset], &node, sizeof(node_cell));
                    update_data_block(data, node_ptr->block_num, &updated);
                    fill_block(data, 0, data->ctrl_block);
                } else if (updated.metadata.type == NODE) {
                    memcpy(&node.last_attribute, &old_cell.prev, sizeof(cell_ptr));
                    memcpy(&((node_block *) &updated)->nodes[node_ptr->offset], &node, sizeof(node_cell));
                    update_data_block(data, node_ptr->block_num, &updated);
                } else {
                    attribute_cell prev_attr = ((attribute_block *) &updated)->attributes[prev.offset];
                    memcpy(&prev_attr.prev, &old_cell.prev, sizeof(cell_ptr));
                    memcpy(&((attribute_block *) &updated)->attributes[prev.offset], &prev_attr,
                           sizeof(attribute_cell));
                    update_data_block(data, prev.block_num, &updated);
                }
                bzero(&read_attr, BLOCK_SIZE);
                fill_block(data, block_number, &read_attr);
                memcpy(&new_cell.prev.block_num, &data->ctrl_block->fragmented_attribute_block, sizeof(int32_t));
                memcpy(&new_cell.prev.offset, &data->ctrl_block->empty_attribute_number, sizeof(int16_t));
                memcpy(&((attribute_block *) &read_attr)->attributes[offset], &new_cell, sizeof(attribute_cell));
                update_data_block(data, block_number, &read_attr);
                memcpy(&data->ctrl_block->fragmented_attribute_block, &block_number, sizeof(int32_t));
                memcpy(&data->ctrl_block->empty_attribute_number, &offset, sizeof(int16_t));
                update_control_block(data);
                if (!has_changed) number++;
                has_changed = true;
            }
            memcpy(&prev, &current, sizeof(cell_ptr));
            block_number = old_cell.prev.block_num;
            offset = old_cell.prev.offset;
        } while (read_attr.metadata.type != CONTROL);
    }
    return number;
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

static void update_control_block(datafile *data) {
    update_data_block(data, 0, data->ctrl_block);
}

static void update_data_block(datafile *data, int32_t block_number, void *block) {
    fseek(data->file, block_number * BLOCK_SIZE, SEEK_SET);
    fwrite(block, BLOCK_SIZE, 1, data->file);
}