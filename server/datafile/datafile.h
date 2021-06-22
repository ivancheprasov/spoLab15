#include <bits/types/FILE.h>
#include <stdint.h>

#ifndef SPOLAB15_DATAFILE_H
#define SPOLAB15_DATAFILE_H

typedef struct {
    FILE* file;

} datafile;

FILE *open_data_file(char *file_path);

#endif //SPOLAB15_DATAFILE_H

