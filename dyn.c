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

void sort(char *input_file, char *output_file, char *data_type, void *data, int data_size, int *success){
    int output_size;
    int *output;
    read_file_integer(input_file,&output, &output_size, success);
    sort_integer_array(output, output_size);

    /*
    new set op
    deliver data

    mpi_bcast
    
    
    
    */
    sleep(1);
    write_integer_to_file(output_file, output, output_size, success);

}


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


















#ifdef __cplusplus
}  // extern "C"
#endif