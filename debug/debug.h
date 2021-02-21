#ifndef _DEBUG_H
#define _DEBUG_H

#include <stdio.h>

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

#define errorf(...)                                                         \
        do { fprintf(stderr, "ERROR: %s:%d: ",__FILE__, __LINE__);           \
             fprintf(stderr, __VA_ARGS__);                                    \
             fprintf(stderr, "\n"); } while (0)

#define infof(...)                                                          \
        do { if (get_debug_level() < DEBUG_INFO) break;                     \
            fprintf(stderr, "INFO: %s:%d: ",__FILE__, __LINE__);            \
             fprintf(stderr, __VA_ARGS__); \
             fprintf(stderr, "\n"); } while (0)

#define warningf(...)                                                       \
        do { if (get_debug_level() < DEBUG_WARNING) break;                  \
            fprintf(stderr, "WARNING: %s:%d: ",__FILE__, __LINE__);             \
             fprintf(stderr, __VA_ARGS__); \
             fprintf(stderr, "\n"); } while (0)

#define debugf(...)                                                         \
        do { if (get_debug_level() < DEBUG_DEBUG) break;                    \
            fprintf(stderr, "DEBUG: %s:%d: ",__FILE__, __LINE__);           \
             fprintf(stderr, __VA_ARGS__); \
             fprintf(stderr, "\n"); } while (0)


#endif /* _DEBUG_H */