#include <stdio.h>
#include <jansson.h>
#include "file_helper.h"
#include <stdbool.h>


void swap(int *xp, int *yp)
{
    int temp = *xp;
    *xp = *yp;
    *yp = temp;
}

void sort_integer_array(int *arr, int n)
{
    int i, j;
    bool swapped;
    for (i = 0; i < n - 1; i++)
    {
        swapped = false;
        for (j = 0; j < n - i - 1; j++)
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

void sort(char *input_file, char *output_file, char *data_type, void *data, int data_size, int *success){
    int output_size;
    int *output;
    read_file_integer(input_file,&output, &output_size, success);

    sort_integer_array(output, output_size);

    write_integer_to_file(output_file, output, output_size, success);

}
