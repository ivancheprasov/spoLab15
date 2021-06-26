#include <stdio.h>
#include "../datafile/cells.h"
#include "../datafile/blocks.h"

int main() {
    printf("Size of %s is %lu\n", "str_ptr", sizeof(cell_ptr));
    printf("Size of %s is %lu\n", "label", sizeof(label_cell));
    printf("Size of %s is %lu\n", "attribute", sizeof(attribute_cell));
    printf("Size of %s is %lu\n", "relation", sizeof(relation_cell));
    printf("Size of %s is %lu\n", "node", sizeof(node_cell));
    printf("Number of labels in block is %lu\n", LABELS_IN_BLOCK);
    printf("Number of nodes in block is %lu\n", NODES_IN_BLOCK);
    printf("Number of nodes in control block is %lu\n", NODES_IN_CONTROL_BLOCK);
    printf("Size of %s is %lu\n", "TYPE", sizeof(TYPE));
    printf("Size of %s is %lu\n", "block_metadata", sizeof(block_metadata));
    printf("Size of %s is %lu\n", "label_block", sizeof(label_block));
    printf("Size of %s is %lu\n", "attribute_block", sizeof(attribute_block));
    printf("Size of %s is %lu\n", "relation_block", sizeof(relation_block));
    printf("Size of %s is %lu\n", "node_block", sizeof(node_block));
    printf("Size of %s is %lu\n", "str_block", sizeof(str_block));
    printf("Size of %s is %lu\n", "control_block", sizeof(control_block));
}
