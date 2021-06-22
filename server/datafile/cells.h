#ifndef SPOLAB15_CELLS_H
#define SPOLAB15_CELLS_H

#include <stdint-gcc.h>

#pragma pack(push, 1)

typedef struct {
    int16_t size;
    char *value;
} str;

typedef struct {
    int16_t offset;
    int32_t block_num;
} str_ptr;//6 bytes

typedef struct {
    str_ptr name;
} label;//6 bytes

typedef struct {
    str_ptr key;
    str_ptr value;
} attribute;//12 bytes

typedef struct {
    str_ptr name;
    int32_t node_b_block;
    int8_t node_b_offset;
} relation;//11 bytes

typedef struct {
    int32_t block_num;
    int8_t offset;
} pack_ptr;//5 bytes

typedef struct {
    pack_ptr first_label;
    pack_ptr first_attribute;
    pack_ptr first_relation;
    pack_ptr empty_label;
    pack_ptr empty_attribute;
    pack_ptr empty_relation;
} node;

#pragma pack(pop)

#endif //SPOLAB15_CELLS_H
