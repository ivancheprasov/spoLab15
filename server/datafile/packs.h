#ifndef SPOLAB15_PACKS_H
#define SPOLAB15_PACKS_H

#include "const.h"
#include <stdint-gcc.h>
#include "cells.h"

#define LABELS_IN_PACK 4
#define ATTRIBUTES_IN_PACK 4
#define RELATIONS_IN_PACK 6

#pragma pack(push, 1)
typedef struct {
    int32_t next_pack_block;
    int8_t next_pack_offset;
} pack_metadata;

typedef struct {
    pack_metadata metadata;
    label values[LABELS_IN_PACK];
} label_pack;

typedef struct {
    pack_metadata metadata;
    attribute values[ATTRIBUTES_IN_PACK];
} attribute_pack;

typedef struct {
    pack_metadata metadata;
    relation values[RELATIONS_IN_PACK];
} relation_pack;
#pragma pack(pop)
//label_pack *labels_pack(void *value);
//
//attribute_pack *attributes_pack(void *value);
//
//relation_pack *relations_pack(void *value);

#endif //SPOLAB15_PACKS_H
