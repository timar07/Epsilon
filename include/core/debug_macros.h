#ifndef EPS_DEBUG_MACROS
#   define EPS_DEBUG_MACROS

#include <stdio.h>

#ifdef EPS_DBG
#   define _DEBUG(format, ...) printf(format, ##__VA_ARGS__);
#else
#   define _DEBUG(format, ...);
#endif

#endif
