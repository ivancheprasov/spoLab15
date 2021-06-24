#ifndef SPOLAB15_RELATION_H
#define SPOLAB15_RELATION_H

#include "cells.h"
#include "datafile.h"

cell_ptr *create_relation_cell(datafile *data, cell_ptr *string_cell, cell_ptr *first_node_cell, cell_ptr *second_node_cell);

#endif //SPOLAB15_RELATION_H
