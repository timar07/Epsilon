#include "core/input.h"
#include "core/memory.h"
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define MAX_LINE_LEN 80

static Eps_Input *src; /* source file */

static size_t get_file_len(FILE *f)
{
	fseek(f, 0, SEEK_END);
	return ftell(f);
}

static void
set_input_name(char *name)
{
	strcpy(src->name, name);
}

void Eps_StartDialog(void (*callback)(Eps_Input*))
{
	src = Eps_AllocMem(sizeof(char)*MAX_LINE_LEN*200); /* TODO: buffer */
	char* line = Eps_AllocMem(sizeof(char)*MAX_LINE_LEN);
	bool is_eof = false;

	set_input_name("stdin");

	while (!is_eof) {
		printf(">>> ");
		is_eof = scanf("%s", line) == EOF;
		src->raw = line;
		(*callback)(src);
	}
}

Eps_Input *Eps_ReadFile(char* fname)
{
	FILE *f;
	size_t flen;
	src = Eps_AllocMem(sizeof(Eps_Input));

	if ((f = fopen(fname, "r")) == NULL) {
		fprintf(stderr, "cannot open file: %s\n", fname);
		exit(1);
	}

	flen = get_file_len(f);
	fseek(f, 0, SEEK_SET);
	src->raw = Eps_AllocMem(flen);
	src->len = flen/sizeof(char);
	set_input_name(fname);

	if (src->raw != NULL) {
		fread(src->raw, 1, flen, f);
	} else {
		fprintf(stderr, "file source malloc failed\n");
		exit(1);
	}

	fclose(f);
	return src;
}
