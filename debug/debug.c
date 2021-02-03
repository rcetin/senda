#include <string.h>

#include "debug.h"

static int debuglevel = DEBUG_ERROR;

void set_debug_level(int level)
{
    if (level == -1) {
        debuglevel = DEBUG_ERROR;
    }

    debuglevel = level;
}

int get_debug_level(void)
{
    return debuglevel;
}

int str2debug(const char *str)
{
    if (!strcmp(str, "debug")) return DEBUG_DEBUG;
    else if (!strcmp(str, "warning")) return DEBUG_WARNING;
    else if (!strcmp(str, "info")) return DEBUG_INFO;
    else if (!strcmp(str, "error")) return DEBUG_ERROR;
    else return -1;
}

void list_debug_levels(void)
{
    fprintf(stderr, "Debug levels: \"debug\", \"warning\", \"info\", \"error\" (default: error)\n");
}