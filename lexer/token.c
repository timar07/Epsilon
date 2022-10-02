#include "lexer/token.h"
#include "core/memory.h"
#include <string.h>
#include <stdio.h>

Eps_Token *
Eps_CreateToken(Eps_LexState *ls, Eps_TokenType toktype, char lexeme[])
{
    Eps_Token* t = EpsMem_Alloc(sizeof(Eps_Token));

    memcpy(&t->ls, ls, sizeof(Eps_LexState));
    t->toktype = toktype;
    t->lexeme = lexeme;

    return t;
}

void
Eps_DestroyToken(Eps_Token *tok)
{
    EpsMem_Free(tok->lexeme);
    EpsMem_Free(tok);
}

char *
_EpsDbg_GetTokenTypeString(Eps_TokenType toktype)
{
    return _EpsDbg_TokenTypeStrings[toktype];
}

#ifdef EPS_DBG

void
_EpsDbg_TokenDump(Eps_Token *tok)
{
    printf("<%s>\n", _EpsDbg_GetTokenTypeString(tok->toktype));
}

#endif
