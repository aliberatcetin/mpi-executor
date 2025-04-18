
#ifndef FILE_HELPER_H
#define FILE_HELPER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void write_integer_to_file(char *file_name, int *data, int data_size,int *success){
    FILE *file;

    char file_[50] = "";
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

    for (int i = 0; i < data_size; ++i) {
        fprintf(file, "%d", data[i]);
        if (i < data_size - 1) {
            fprintf(file, " ");
        }
    }

    fclose(file);
}

void read_file_in_bulk(const char *file_name, int **output, int *output_size, int *success) {
    char file_[50] = "";
    strcat(file_, "./data/");
    strcat(file_,file_name);
    strcat(file_,"\0");

    FILE *file = fopen(file_, "r");
    if (file == NULL) {
        perror("Unable to open file");
        *success = 0;
        return;
    }

    // Get the file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Allocate memory for the file content
    char *buffer = (char *)malloc(file_size + 1);
    if (buffer == NULL) {
        perror("Memory allocation failed");
        *success = 0;
        fclose(file);
        return;
    }

    // Read the file content into the buffer
    size_t read_size = fread(buffer, 1, file_size, file);
    buffer[read_size] = '\0'; // Null-terminate the buffer

    fclose(file);

    // Count the number of integers
    int count = 0;
    for (size_t i = 0; i < read_size; i++) {
        if (buffer[i] == ' ' || buffer[i] == '\n') {
            count++;
        }
    }
    count++; // For the last integer

    // Allocate memory for the integers
    int *data = (int *)malloc(count * sizeof(int));
    if (data == NULL) {
        perror("Memory allocation for integers failed");
        *success = 0;
        free(buffer);
        return;
    }

    // Parse the integers from the buffer
    char *ptr = buffer;
    int index = 0;
    while (*ptr) {
        data[index++] = strtol(ptr, &ptr, 10);
        while (*ptr == ' ' || *ptr == '\n') {
            ptr++;
        }
    }

    // Clean up and set output values
    free(buffer);
    *output_size = count;
    *output = data;
    *success = 1;
}

void read_file_integer(char *file_name, int **output, int *output_size,int *success){

    int *data = (int *)malloc(sizeof(int) * 30000010);
    if(data==NULL){
        printf("DATA NULL\n");
    }else{
        printf("NOT NULL\n");
    }
    for(int i=0;i<30000000;i++){
        data[i]=i;
    }
    printf("DONE\n");
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
            //data = (int *)realloc(data, sizeof(int) * 100 + 100);
        }
        if (n >= 30000010) {
            fprintf(stderr, "Exceeded allocated memory size\n");
            *success = 0;
            fclose(file);
            return;
        }
    }

    *output_size = n;
    *output = data;
    fclose(file);
}



char* int_array_to_string(int *data, int data_size) {
    // Estimate the buffer size
    size_t buffer_size = data_size * 12; // Assuming each integer can be up to 10 digits, plus space and null terminator
    char *buffer = (char *)malloc(buffer_size);
    if (buffer == NULL) {
        perror("Memory allocation failed");
        return NULL;
    }

    char *ptr = buffer;
    for (int i = 0; i < data_size; ++i) {
        ptr += sprintf(ptr, "%d", data[i]);
        if (i < data_size - 1) {
            *ptr++ = ' ';
        }
    }
    *ptr = '\0'; // Null-terminate the string

    return buffer;
}

// Function to write the space-separated integer string to a file
void write_file_in_bulk(const char *file_name, int *data, int data_size, int *success) {

    // Convert integer array to space-separated string
    char *data_string = int_array_to_string(data, data_size);
    if (data_string == NULL) {
        *success = 0;
        return;
    }

    // Open the file for writing
    char file_[50] = "";
    strcat(file_, "./data/");
    strcat(file_,file_name);
    strcat(file_,"\0");
    printf("%s attempt to write file\n", file_);

    FILE *file = fopen(file_, "w");
    if (file == NULL) {
        perror("Unable to open file for writing");
        free(data_string);
        *success = 0;
        return;
    }

    // Write the data string to the file
    size_t write_size = fwrite(data_string, sizeof(char), strlen(data_string), file);
    if (write_size < strlen(data_string)) {
        perror("Error writing to file");
        *success = 0;
    } else {
        *success = 1;
    }

    // Clean up
    free(data_string);
    fclose(file);
}







#endif