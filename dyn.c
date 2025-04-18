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
        write_file_in_bulk(target2, &data[start], end - start, &success);

       
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
    read_file_in_bulk(input_file,&output, &output_size, success);
    
    //sort_integer_array(output, output_size);

    /*int num_integers = 1000;

    // Allocate memory for the array
    int *helo = (int *)malloc(num_integers * sizeof(int));
    if (helo == NULL) {
        perror("MMMemory allocation failed");
        return 1;
    }

    // Fill the array with integers from 1 billion to 1
    for (int i = 0; i < num_integers; i++) {
        helo[i] = num_integers - i;
    }
     printf("First 10 values: ");
    for (int i = 0; i < 10; i++) {
        printf("%d ", helo[i]);
    }
    printf("\n");

    printf("Last 10 values: ");
    for (int i = num_integers - 10; i < num_integers; i++) {
        printf("%d ", helo[i]);
    }
    printf("\n");*/
    
    distribute_data_to_targets(root, output, output_size);

    /*
    new set op
    deliver data

    mpi_bcast
    
    */
    //sleep(1);
    //write_integer_to_file(output_file, output, output_size, success);

}

void sortFile(json_t *root, char *input_file, char *output_file, char *data_type, void *data, int data_size, int *success){
    int output_size;
    int *output;
    
    
    read_file_in_bulk(input_file,&output, &output_size, success);
    printf("AFTER READ %d %d\n",output_size,*success);
    mergeSort(output, 0, output_size-1);
    
    //sort_integer_array(output, output_size);
    //sleep(rand()%20+30);
    write_file_in_bulk(output_file, output, output_size, success);
}

void sort(json_t *root, char *input_file, char *output_file, char *data_type, void *data, int data_size, int *success){
    int output_size;
    int *output;
    char input_file2[100] = "";
    strcpy(input_file2,"dist");
    strcat(input_file2,output_file);
    //strcat(input_file2,".txt");
    strcat(input_file2,"\0");
    read_file_in_bulk(input_file2,&output, &output_size, success);
    printf("starting sort %d\n",output_size);
    //mergeSort(output, output_size-1);
    printf("finished sort %d\n",output_size);
    sort_integer_array(output, output_size);
    //sleep(rand()%20+30);
    write_file_in_bulk(output_file, output, output_size, success);

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

    int **data_arrays = (int **)malloc(num_targets * sizeof(int *));
    int *sizes = (int *)malloc(num_targets * sizeof(int));
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
    
        read_file_in_bulk(target2, &data_arrays[index], &sizes[index], success);
        if (!success) {
            fprintf(stderr, "Failed to read file %s\n", files[index]);
            return;
        }
        printf("%s\n", target2);
        
    }
   

    printf("REDUCE RESULT %d %d %d\n", sizes[0],data_arrays[0][0],success);

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
    write_file_in_bulk(output_file, merged_data, merged_size, success);
    if (!success) {
        fprintf(stderr, "Failed to write to output file %s\n", output_file);
    }

    // Free allocated memory

}


void multiplyByTwo(json_t *root, char *input_file, char *output_file, char *data_type, void *data, int data_size, int *success){
    int output_size;
    int *output;
    read_file_in_bulk(input_file,&output, &output_size, success);
    multiply(output, output_size, 2);
    write_integer_to_file(output_file, output, output_size, success);
}

/*
void multiplyByThree(char *input_file, char *output_file, char *data_type, void *data, int data_size, int *success){
    int output_size;
    int *output;
    read_file_integer(input_file,&output, &output_size, success);
    multiply(output, output_size, 3);
    sleep(1);
    write_integer_to_file(output_file, output, output_size, success);

}
*/










// Function to merge two halves
void merge(int arr[], int l, int m, int r) {
    int i, j, k;
    int n1 = m - l + 1;
    int n2 = r - m;

    // Create temp arrays
    int L[n1], R[n2];

    // Copy data to temp arrays L[] and R[]
    for (i = 0; i < n1; i++)
        L[i] = arr[l + i];
    for (j = 0; j < n2; j++)
        R[j] = arr[m + 1 + j];

    // Merge the temp arrays back into arr[l..r]
    i = 0; // Initial index of first subarray
    j = 0; // Initial index of second subarray
    k = l; // Initial index of merged subarray
    while (i < n1 && j < n2) {
        if (L[i] <= R[j]) {
            arr[k] = L[i];
            i++;
        } else {
            arr[k] = R[j];
            j++;
        }
        k++;
    }

    // Copy the remaining elements of L[], if there are any
    while (i < n1) {
        arr[k] = L[i];
        i++;
        k++;
    }

    // Copy the remaining elements of R[], if there are any
    while (j < n2) {
        arr[k] = R[j];
        j++;
        k++;
    }
}

// Function to implement merge sort
void mergeSort(int arr[], int l, int r) {
    if (l < r) {
        // Same as (l+r)/2, but avoids overflow for large l and r
        int m = l + (r - l) / 2;

        // Sort first and second halves
        mergeSort(arr, l, m);
        mergeSort(arr, m + 1, r);   

        merge(arr, l, m, r);
    }
}

int min(int x, int y) { return (x<y)? x :y; }
 
 
/* Iterative mergesort function to sort arr[0...n-1] */
void mergeSort2(int arr[], int n)
{
   int curr_size;  // For current size of subarrays to be merged
                   // curr_size varies from 1 to n/2
   int left_start; // For picking starting index of left subarray
                   // to be merged
 
   // Merge subarrays in bottom up manner.  First merge subarrays of
   // size 1 to create sorted subarrays of size 2, then merge subarrays
   // of size 2 to create sorted subarrays of size 4, and so on.
   for (curr_size=1; curr_size<=n-1; curr_size = 2*curr_size)
   {
       // Pick starting point of different subarrays of current size
       for (left_start=0; left_start<n-1; left_start += 2*curr_size)
       {
           // Find ending point of left subarray. mid+1 is starting 
           // point of right
           int mid = min(left_start + curr_size - 1, n-1);
 
           int right_end = min(left_start + 2*curr_size - 1, n-1);
 
           // Merge Subarrays arr[left_start...mid] & arr[mid+1...right_end]
           merge2(arr, left_start, mid, right_end);
       }
   }
}
 
/* Function to merge the two haves arr[l..m] and arr[m+1..r] of array arr[] */
void merge2(int arr[], int l, int m, int r)
{
    int i, j, k;
    int n1 = m - l + 1;
    int n2 =  r - m;
 
    /* create temp arrays */
    int L[n1], R[n2];
 
    /* Copy data to temp arrays L[] and R[] */
    for (i = 0; i < n1; i++)
        L[i] = arr[l + i];
    for (j = 0; j < n2; j++)
        R[j] = arr[m + 1+ j];
 
    /* Merge the temp arrays back into arr[l..r]*/
    i = 0;
    j = 0;
    k = l;
    while (i < n1 && j < n2)
    {
        if (L[i] <= R[j])
        {
            arr[k] = L[i];
            i++;
        }
        else
        {
            arr[k] = R[j];
            j++;
        }
        k++;
    }
 
    /* Copy the remaining elements of L[], if there are any */
    while (i < n1)
    {
        arr[k] = L[i];
        i++;
        k++;
    }
 
    /* Copy the remaining elements of R[], if there are any */
    while (j < n2)
    {
        arr[k] = R[j];
        j++;
        k++;
    }
}



#ifdef __cplusplus
}  // extern "C"
#endif








