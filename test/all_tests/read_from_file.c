#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "all_tests.h"

const char *read_data_from_file(const char *filename)
{
    int alloc_size = 1024;
    int remaining = alloc_size;
    int idx = 0;
    size_t read;

    char *buffer = calloc(alloc_size, sizeof(char));
    if (!buffer) {
        perror("[JSON] alloc failed");
        return NULL;
    }

    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("[JSON] open failed");
        return NULL;
    }

    do {
        read = fread(buffer + idx, 1, remaining, fp);
        if (!read) {
            // return buffer;
            fprintf(stderr, "Read file completed.\n\n");
            break;
        }

        remaining -= read;
        idx += read;
        if (!remaining) {
            char *new_buf = realloc(buffer, 2 * alloc_size);
            if (!new_buf) {
                perror("[JSON] realloc failed");
                free(buffer);
                return NULL;
            }

            remaining += alloc_size;
            alloc_size *= 2;
            fprintf(stderr, "enlarge the buffer, new size=%d\n", alloc_size);
            buffer = new_buf;
        }
    } while(1);

    fprintf(stderr, "Dump file: \n%s\n", buffer);
    return buffer;
}
