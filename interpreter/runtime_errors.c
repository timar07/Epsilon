#include "interpreter/runtime_errors.h"
#include "core/errors.h"
#include <stdio.h>
#include <stdarg.h>

void
EpsErr_RuntimeError(Eps_LexState *ls, char *format, ...)
{
    ERR_INSTANCE_INIT_BUFFER();

    EpsErr_Raise(ls, "Runtime Error", buffer);
}
