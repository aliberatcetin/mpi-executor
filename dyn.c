#include <stdio.h>
#include <jansson.h>
#include "file_helper.h"
#include <stdbool.h>
#include <unistd.h>
//#include "dmr.h"

#include <assert.h>
#include <math.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

void swap(int *xp, int *yp){
    int temp = *xp;
    *xp = *yp;
    *yp = temp;
}

void sort_integer_array(int *arr, int n)
{
    bool swapped;
    for (int i = 0; i < n - 1; i++){
        swapped = false;
        for (int j = 0; j < n - i - 1; j++)
        {
            if (arr[j] > arr[j + 1])
            {
                swap(&arr[j], &arr[j + 1]);
                swapped = true;
            }
        }

        if (swapped == false)
            break;
    }
}
void multiply(int *arr, int n, int multiplicant){
    for (int i = 0; i < n; i++){
        arr[i] = arr[i] *2;
    }
}

void distribute_data_to_targets(json_t *root, int *data, int data_size) {
    json_error_t error;
    size_t index;
    json_t *value;
    int success;
    

    // Get the "targets" array from the root object
    json_t *targets = json_object_get(root, "targets");
    if (!json_is_array(targets)) {
        fprintf(stderr, "error: targets is not an array\n");
        json_decref(root);
        return;
    }

    size_t num_targets = json_array_size(targets);
    printf("%d num targetss\n",num_targets);
    size_t chunk_size = data_size / num_targets;
    size_t remaining = data_size % num_targets;

    // Iterate over the elements in the "targets" array
    json_array_foreach(targets, index, value) {
        // Ensure each element is a string
        if (!json_is_string(value)) {
            fprintf(stderr, "error: array element %zu is not a string\n", index);
            json_decref(root);
            return;
        }

        const char *target = json_string_value(value);
        char target2[100] = "";
        strcat(target2,"dist");
        strcat(target2,target);
        strcat(target2,".txt");
        strcat(target2,"\0");
        size_t start = index * chunk_size;
        size_t end = start + chunk_size  + (index < remaining ? 1 : 0);
        printf("%s %d %d targetbe\n", target2, start, end);
        write_integer_to_file(target2, &data[start], end - start, &success);

       
    }
    json_decref(targets);

}

void merge_arrays(int *arr1, int size1, int *arr2, int size2, int **output, int *output_size) {
    int *merged = (int *)malloc((size1 + size2) * sizeof(int));
    int i = 0, j = 0, k = 0;

    while (i < size1 && j < size2) {
        if (arr1[i] <= arr2[j]) {
            merged[k++] = arr1[i++];
        } else {
            merged[k++] = arr2[j++];
        }
    }
    while (i < size1) {
        merged[k++] = arr1[i++];
    }
    while (j < size2) {
        merged[k++] = arr2[j++];
    }

    *output = merged;
    *output_size = size1 + size2;
}

void map(json_t *root, char *input_file, char *output_file, char *data_type, void *data, int data_size, int *success){
    int output_size;
    int *output;
    read_file_integer(input_file,&output, &output_size, success);
    //sort_integer_array(output, output_size);

    
    distribute_data_to_targets(root, output, output_size);

    /*
    new set op
    deliver data

    mpi_bcast
    
    */
    sleep(1);
    //write_integer_to_file(output_file, output, output_size, success);

}


void sort(json_t *root, char *input_file, char *output_file, char *data_type, void *data, int data_size, int *success){
    int output_size;
    int *output;
    char input_file2[100] = "";
    strcpy(input_file2,"dist");
    strcat(input_file2,output_file);
    //strcat(input_file2,".txt");
    strcat(input_file2,"\0");
    read_file_integer(input_file2,&output, &output_size, success);

    sort_integer_array(output, output_size);
    sleep(1);
    write_integer_to_file(output_file, output, output_size, success);

}

void reduce(json_t *root, char *input_file, char *output_file, char *data_type, void *data, int data_size, int *success){
    json_error_t error;
    size_t index;
    json_t *value;
    

    // Get the "targets" array from the root object
    json_t *targets = json_object_get(root, "dependenciesString");
    if (!json_is_array(targets)) {
        fprintf(stderr, "error: targets is not an array\n");
        json_decref(root);
        return;
    }

    size_t num_targets = json_array_size(targets);
    char **files = (char**)malloc(num_targets);

    printf("%d num deps\n",num_targets);


    // Iterate over the elements in the "targets" array
    json_array_foreach(targets, index, value) {
        // Ensure each element is a string
        if (!json_is_string(value)) {
            fprintf(stderr, "error: array element %zu is not a string\n", index);
            json_decref(root);
            return;
        }

        const char *target = json_string_value(value);
        char target2[100] = "";
        strcat(target2,target);
        strcat(target2,".txt");
        strcat(target2,"\0");
        files[index] = (char*)malloc(100);
        strcpy(files[index],target2);
        printf("%s\n", target2);
        
    }
    json_decref(targets);

    int **data_arrays = (int **)malloc(num_targets * sizeof(int *));
    int *sizes = (int *)malloc(num_targets * sizeof(int));

    // Read all input files
    for (int i = 0; i < num_targets; ++i) {
        read_file_integer(files[i], &data_arrays[i], &sizes[i], success);
        if (!success) {
            fprintf(stderr, "Failed to read file %s\n", files[i]);
            return;
        }
    }


    int *merged_data = data_arrays[0];
    int merged_size = sizes[0];
    for (int i = 1; i < num_targets; ++i) {
        int *new_merged_data;
        int new_merged_size;
        merge_arrays(merged_data, merged_size, data_arrays[i], sizes[i], &new_merged_data, &new_merged_size);
        if (i > 1) {
            free(merged_data);
        }
        merged_data = new_merged_data;
        merged_size = new_merged_size;
    }

    // Write the merged array to the output file
    write_integer_to_file(output_file, merged_data, merged_size, success);
    if (!success) {
        fprintf(stderr, "Failed to write to output file %s\n", output_file);
    }

    // Free allocated memory
    free(merged_data);
    for (int i = 1; i < num_targets; ++i) {
        free(data_arrays[i]);
    }
    free(data_arrays);
    free(sizes);
}

/*

void multiplyByTwo(char *input_file, char *output_file, char *data_type, void *data, int data_size, int *success){
    int output_size;
    int *output;
    read_file_integer(input_file,&output, &output_size, success);
    multiply(output, output_size, 2);
    sleep(1);
    write_integer_to_file(output_file, output, output_size, success);

}

void multiplyByThree(char *input_file, char *output_file, char *data_type, void *data, int data_size, int *success){
    int output_size;
    int *output;
    read_file_integer(input_file,&output, &output_size, success);
    multiply(output, output_size, 3);
    sleep(1);
    write_integer_to_file(output_file, output, output_size, success);

}
*/


















#ifdef __cplusplus
}  // extern "C"
#endif



