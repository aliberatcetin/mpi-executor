
#ifndef FILE_HELPER_H
#define FILE_HELPER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef void (*fptr)(json_t *root, char *input_file, char *output_file, char *data_type, void *data, int data_size, int *success);
#endif