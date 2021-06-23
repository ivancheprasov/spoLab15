#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "../utils/const.h"
#include "server.h"
#include "../datafile/datafile.h"

int main(int argc, char **argv) {
    if (argc < 3) {
        puts("No required arguments provided: <server_port> <data_file>");
        return -1;
    }
    datafile *data = init_data(argv[2]);
    uint16_t port = strtoul(argv[1], NULL, BASE_10);
    server_info *info_ptr = startup(port, data);

    if (info_ptr == NULL) {
        puts("Unable to startup");
        return -1;
    }
    char input[50] = {0};
    do {
        fgets(input, 50, stdin);
    } while(strcmp(input, "q") != 0);
    close_server(info_ptr);
    fclose(data->file);
    free(info_ptr);
}