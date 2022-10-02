#include "interpreter/interpret.h"
#include "parser.h"
#include "lexer/lexer.h"
#include "core/ds/dict.h"
#include "core/ds/list.h"
#include "core/errors.h"
#include "core/memory.h"
#include <stdio.h>
#include <sys/time.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        EpsErr_Fatal("no input file provided");
    }

#ifdef EPS_DBG
    struct timeval t1, t2;
    double elapsedTime;

    gettimeofday(&t1, NULL);
#endif

    Eps_Input *input = Eps_ReadFile(argv[1]);
    EpsList *toks = Eps_Lex(input);
    EpsList *stmts = Eps_Parse(toks);

    Eps_Interpret(stmts);

#ifdef EPS_DBG
    gettimeofday(&t2, NULL);

    elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0;
    elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000.0;

     printf("Finished in: %f ms.\n", elapsedTime);
#endif

     return 0;
}
