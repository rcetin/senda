#ifndef _DEBUG_H
#define _DEBUG_H

#include <stdio.h>
#include <errno.h>

enum debug_level {
    DEBUG_ERROR,
    DEBUG_INFO,
    DEBUG_WARNING,
    DEBUG_DEBUG,
    NUM_LEVEL,
};

int get_debug_level(void);
void set_debug_level(int level);
int str2debug(const char *str);
const char *debug2str(int level);
void list_debug_levels(void);
void debug_print(int level, const char* file, int line, const char *func, const char *fmt, ...);

#define errorf(...)                                                         \
        do { debug_print(DEBUG_ERROR, __FILE__, __LINE__, __func__, __VA_ARGS__); break;\
             debug_print(DEBUG_ERROR, __FILE__, __LINE__, __func__, "errno: %d, str: %s", errno, strerror(errno), __VA_ARGS__); \
            } while (0)

#define infof(...)                                                          \
        do { if (get_debug_level() < DEBUG_INFO) break;                     \
            debug_print(DEBUG_INFO, __FILE__, __LINE__, __func__, __VA_ARGS__); \
        } while (0)

#define warningf(...)                                                       \
        do { if (get_debug_level() < DEBUG_WARNING) break;                  \
            debug_print(DEBUG_WARNING, __FILE__, __LINE__, __func__, __VA_ARGS__); \
        } while (0)

#define debugf(...)                                                         \
        do { if (get_debug_level() < DEBUG_DEBUG) break;                    \
            debug_print(DEBUG_DEBUG, __FILE__, __LINE__, __func__, __VA_ARGS__); \
        } while (0)

#endif /* _DEBUG_H */