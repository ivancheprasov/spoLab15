#ifndef SPOLAB15_CELLS_H
#define SPOLAB15_CELLS_H

#include <stdint.h>

#pragma pack(push, 1)

typedef struct {
    int16_t size;
    char *value;
} str;

typedef struct {
    int32_t block_num;
    int16_t offset;
} cell_ptr;//6 bytes

typedef struct {
    cell_ptr prev;
    cell_ptr name;
} label_cell;//12 bytes

typedef struct {
    cell_ptr prev;
    cell_ptr key;
    cell_ptr value;
} attribute_cell;//18 bytes

typedef struct {
    cell_ptr prev;
    cell_ptr name;
    int32_t node_b_block;
    int8_t node_b_offset;
} relation_cell;//17 bytes

typedef struct {
    cell_ptr last_label;
    cell_ptr last_attribute;
    cell_ptr last_relation;
} node_cell; //18 bytes

#pragma pack(pop)

#endif //SPOLAB15_CELLS_H
