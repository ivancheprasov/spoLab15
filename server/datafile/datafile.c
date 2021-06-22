#include <unistd.h>
#include <stdio.h>
#include "datafile.h"

FILE *open_data_file(char *filepath) {
    FILE *data_file;
    data_file = fopen(filepath, access(filepath, F_OK) == 0 ? "rb+" : "wb");
    return data_file;
}
