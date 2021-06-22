#ifndef SPOLAB15_BLOCKS_H
#define SPOLAB15_BLOCKS_H

#include "cells.h"
#include "const.h"
#include "packs.h"

#define LABEL_PACKS_IN_BLOCK ((BLOCK_SIZE-sizeof(block_metadata))/sizeof(label_pack))
#define ATTRIBUTE_PACKS_IN_BLOCK ((BLOCK_SIZE-sizeof(block_metadata))/sizeof(attribute_pack))
#define RELATION_PACKS_IN_BLOCK ((BLOCK_SIZE-sizeof(block_metadata))/sizeof(relation_pack))
#define NODES_IN_BLOCK ((BLOCK_SIZE-sizeof(block_metadata))/sizeof(node))

#pragma pack(push, 1)

typedef enum TYPE {
    NODE,
    LABEL,
    ATTRIBUTE,
    RELATION,
    STRING,
    EMPTY
} block_types;

typedef enum TYPE TYPE;

typedef struct {
    TYPE type;
    int32_t next_block;
} block_metadata;

typedef struct {
    block_metadata metadata;
    char reserved;
    label_pack packs[LABEL_PACKS_IN_BLOCK];
} label_block;

typedef struct {
    block_metadata metadata;
    char reserved[9];
    attribute_pack packs[ATTRIBUTE_PACKS_IN_BLOCK];
} attribute_block;

typedef struct {
    block_metadata metadata;
    char reserved[22];
    relation_pack packs[RELATION_PACKS_IN_BLOCK];
} relation_block;

typedef struct {
    block_metadata metadata;
    int16_t free_space_ptr;
    char data[1014];
} str_block;

typedef struct {
    block_metadata metadata;
    int32_t first_empty_block;
    int32_t fragmented_label_block;
    int32_t fragmented_attribute_block;
    int32_t fragmented_relation_block;
    int32_t fragmented_string_block;
    char reserved[6];
    node nodes[NODES_IN_BLOCK];
} node_block;

#pragma pack(pop)

#endif //SPOLAB15_BLOCKS_H
