#ifndef EPS_ERRORS
#   define EPS_ERRORS

#include "lexer/token.h"
#include <stdbool.h>

#define MAX_ERROR_MSG_LEN 80

#define ERR_INSTANCE_INIT_BUFFER() \
    char buffer[1024]; \
    va_list arg; \
    if (format) { \
        va_start (arg, format); \
        vsnprintf(buffer, sizeof(buffer), format, arg); \
        va_end (arg); \
    } \

void
EpsErr_Raise(Eps_LexState *ls, const char errname[], const char msg[]);

bool
EpsErr_WasError(void);

void
EpsErr_Fatal(const char msg[]);

#endif
