
#ifndef FILE_HELPER_H
#define FILE_HELPER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void write_integer_to_file(char *file_name, int *data, int data_size,int *success){
    FILE *file;

    char file_[50];
    strcat(file_, "./data/");
    strcat(file_,file_name);
    strcat(file_,"\0");
    printf("%s attempt to write file\n", file_);
    file = fopen(file_, "w");
    if (file == NULL) {
        file = fopen(file_, "wb"); 
        if(file==NULL){
            perror("Unable to open file write");
            *success = 0;
            return;
        }
    }

    for (int i = 0; i < data_size; i++) {
        fprintf(file, "%d ", data[i]); 
    }

    fclose(file);
}


void read_file_integer(char *file_name, int **output, int *output_size,int *success)
{

    int *data = (int *)malloc(sizeof(int) * 100);

    FILE *file;
    int n = 0;
    char file_[50] = "";
    strcat(file_, "./data/");
    strcat(file_,file_name);
    strcat(file_,"\0");

    printf("%s attempt to read file\n", file_);


    file = fopen(file_, "r");
    if (file == NULL)
    {
        perror("Unable to open file read");
        *success = 0;
        return;
    }

    while (fscanf(file, "%d", &data[n]) == 1)
    {
        n++;
        if (n >= 100)
        {
            data = (int *)realloc(data, sizeof(int) * 100 + 100);
        }
    }

    *output_size = n;
    *output = data;
    fclose(file);
}


#endif