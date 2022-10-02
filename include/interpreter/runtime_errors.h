#ifndef _RUNTIME_ERRORS_H
#   define _RUNTIME_ERRORS_H

#include "lexer/token.h"

void
EpsErr_RuntimeError(Eps_LexState *ls, char *format, ...);

#endif