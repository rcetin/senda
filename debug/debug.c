#include <string.h>

#include "debug.h"

static int debuglevel = DEBUG_ERROR;

void set_debug_level(int level)
{
    if (level == -1) {
        debuglevel = DEBUG_ERROR;
    }

    debuglevel = level;
    errorf("Setting debug to %s", debug2str(get_debug_level()));
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

const char *debug2str(int level)
{
    if (level == DEBUG_DEBUG) return "debug";
    else if (level == DEBUG_WARNING) return "warning";
    else if (level == DEBUG_INFO) return "info";
    else if (level == DEBUG_ERROR) return "error";
    else return "undefined";
}

void list_debug_levels(void)
{
    fprintf(stderr, "Debug levels: \"debug\", \"warning\", \"info\", \"error\" (default: error)\n");
}