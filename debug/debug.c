#define _GNU_SOURCE

#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <time.h>

#include "debug.h"
#include "utils/utils.h"

/******* Color Codes *******/
#define BLK "\e[0;30m"
#define RED "\e[0;31m"
#define GRN "\e[0;32m"
#define YEL "\e[0;33m"
#define BLU "\e[0;34m"
#define MAG "\e[0;35m"
#define CYN "\e[0;36m"
#define WHT "\e[0;37m"

#define BRED "\e[1;31m"

#define RST "\e[0m"
/**************************/

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

void debug_print(int level, const char* file, int line, const char *func, const char *fmt, ...)
{
    int size = 0;
    int rc;
    char *p = NULL;
    time_t rawtime;
    struct tm * timeinfo;
    char date[128] = {0};
    va_list ap;
    struct {
        const char *name;
        const char *color;
        int date_included;
    } debug_ctx[NUM_LEVEL] =    {{"ERR", BRED, 1},
                                {"INFO", RST, 1},
                                {"WARN", YEL, 1},
                                {"DBG", RST, 1}};

    va_start(ap, fmt);
    rc = vasprintf(&p, fmt, ap);

    if (rc < 0) {
        fprintf(stderr, "memory allocation error");
        return;
    }

    time(&rawtime);
    timeinfo = localtime(&rawtime);
    snprintf(date, sizeof(date), "%02d.%02d.%02d %02d:%02d:%02d", timeinfo->tm_mday, timeinfo->tm_mon, timeinfo->tm_year + 1900,
                                                                    timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
    


    fprintf(stderr, "%s[%s]:\t%s [%s:%d][%s()] %s\n"RST, debug_ctx[level].color,
                                                        debug_ctx[level].name,
                                                        debug_ctx[level].date_included ? date : "",
                                                        file,
                                                        line,
                                                        func,
                                                        p);

    va_end(ap);

    free(p);
}