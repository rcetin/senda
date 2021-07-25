#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

#include "all_tests.h"

const char *read_data_from_file(const char *filename)
{
    int alloc_size = 1024;
    int remaining = alloc_size;
    int idx = 0;
    size_t read;
    long sz = 0;
    FILE *fp = NULL;
    void *map_addr = NULL;

    fp = fopen(filename, "r");
    if (!fp) {
        perror("[JSON] open failed");
        return NULL;
    }

    fseek(fp, 0L, SEEK_END);
    sz = ftell(fp);

    map_addr = mmap(NULL, sz, PROT_READ, MAP_PRIVATE, fileno(fp), 0);
    if (map_addr == MAP_FAILED) {
        perror("file mapping is failed!\n");
        return NULL;
    }

    fclose(fp);

    size_t t = strlen((const char *)map_addr);
    
    // fprintf(stderr, "Dump file: \n%s\n", (const char *)map_addr);
    return map_addr;
}
