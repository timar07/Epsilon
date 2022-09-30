#ifndef INPUT_H
#define INPUT_H
#include <stddef.h> /* size_t */

#define MAX_FNAME_LEN 255

typedef struct {
    char *raw;
    size_t len;
    char name[MAX_FNAME_LEN];
} Eps_Input;

Eps_Input *Eps_ReadFile(char *fname);

/**
 * Starts dialog mode, all IO API
 * works the same.
 */
void Eps_StartDialog(void (*callback)(Eps_Input*));

#endif /* end INPUT_H */