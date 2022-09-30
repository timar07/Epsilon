#include "core/errors.h"
#include "core/state.h"
#include "core/memory.h"
#include "lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define RED_STR(str) "\x1B[31m"str"\x1B[0m"
#define ERR_INDENT 4

static bool had_error = false;

bool
EpsErr_WasError(void)
{
    return had_error;
}

static void
print_error(Eps_LexState *ls, const char errname[], const char msg[])
{
    fprintf(
        stderr,
        "%s {%lu:%lu} "RED_STR("%s:")"\n%*s%s\n",
        ls->fname,
        ls->line,
        ls->col,
        errname,
        ERR_INDENT,
        "",
        msg
    );
}

void
print_context(Eps_LexState *ls)
{
    // printing out the line
	size_t current_line = 1;
	size_t strsz;
	size_t start = 0;
	size_t end;
	char *line;

	while (current_line < ls->line) {
		if (ls->input->raw[start++] == '\n') {
			current_line++;
		}
	}

	end = start;

	while (ls->input->raw[end] != '\n' && end < ls->input->len)
        end++;


	strsz = end - start + 1;

	line = Eps_AllocMem(sizeof(char)*strsz);
	memcpy(line, &ls->input->raw[start], strsz-1);
	line[strsz] = '\0';

    printf(
        "%*s%s\n",
        ERR_INDENT,
        "",
        line
    );

    // printing out underline
    size_t current = start;

    printf("%*s", ERR_INDENT, "");

    while (current++ < ls->start) {
        printf(" ");
    }

    while (current++ < ls->end) {
        printf(RED_STR("~"));
    }

    printf(RED_STR("^")"\n");
}

void
EpsErr_Raise(Eps_LexState *ls, const char errname[], const char msg[])
{
    print_error(ls, errname, msg);
    print_context(ls);

    had_error = true;
}

void EpsErr_Fatal(const char msg[])
{
    fprintf(stderr, RED_STR("Fatal: %s\n"), msg);
    exit(1);
}
