#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <mpi.h>
#include <pthread.h>
#include <jansson.h>
#include <curl/curl.h>
#include <stdbool.h>

#define SOCKET_PATH "/tmp/mpi_socket"
#define BUFFER_SIZE 5000
#define WORK_DONE_TAG 1
#define WORK_TAG 2
pthread_mutex_t mutex;

struct string {
    char *ptr;
    size_t len;
};

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

void* listen_for_availability(void* arg) {
    int* available = (int*)arg;
    MPI_Status status;
    printf("listener thread started\n");
    int flag;

    while (1) {
        MPI_Recv(NULL, 0, MPI_INT, MPI_ANY_SOURCE, WORK_DONE_TAG, MPI_COMM_WORLD, &status);
        pthread_mutex_lock(&mutex); 
        printf("%d done thread lock\n",status.MPI_SOURCE);
        available[status.MPI_SOURCE] = 1;
        pthread_mutex_unlock(&mutex);
        printf("thread unlock\n");
    }


    return NULL;
}

void read_file_integer(char *file_name, int **output, int *output_size)
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
        perror("Unable to open file");
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

void write_to_file_integer(char *file_name, int *data, int data_size){
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
            perror("Unable to open file");
            return;
        }
    }

    for (int i = 0; i < data_size; i++) {
        fprintf(file, "%d ", data[i]); 
    }

    fclose(file);
}

void sort_manager(char *input_file, char *output_file, char *data_type, void *data){
    if (input_file != NULL){
        if (strcmp(data_type, "INTEGER") == 0){
            int *data_ = (int *)data;
            int data_size = 0;
            read_file_integer(input_file,&data_,&data_size);
            sort_integer_array(data_, data_size);
            write_to_file_integer(output_file, data_, data_size);
        }
    }
}


void execute(char *task_type, char *input_file, char *output_file, char *data_type, void *data, int data_size){
    printf("task type %s ,s being executed\n", task_type);

    if (strcmp(task_type, "sort") == 0){
        sort_manager(input_file, output_file, data_type, data);
    }

    printf("task type: %s is terminated", task_type);
}

void pipeline(char *type, char *input_file, char *output_file, char *data_type, void *data, int data_size,char *inputSource, char *runLog,int *success){
    // controls
    if (strcmp(inputSource , "file") == 0 && strlen(input_file)<=4){
        *success = 0;
        strcpy(runLog, "no input file");
        return;
    }

    execute(type, input_file, output_file, data_type, data, data_size);
    *success=1;
}


void init_string(struct string *s) {
    s->len = 0;
    s->ptr = (char*)malloc(s->len + 1);
    if (s->ptr == NULL) {
        fprintf(stderr, "malloc() failed\n");
        exit(EXIT_FAILURE);
    }
    s->ptr[0] = '\0';
}
size_t writefunc(void *ptr, size_t size, size_t nmemb, struct string *s) {
    size_t new_len = s->len + size * nmemb;
    s->ptr = (char*)realloc(s->ptr, new_len + 1);
    if (s->ptr == NULL) {
        fprintf(stderr, "realloc() failed\n");
        exit(EXIT_FAILURE);
    }
    memcpy(s->ptr + s->len, ptr, size * nmemb);
    s->ptr[new_len] = '\0';
    s->len = new_len;

    return size * nmemb;
}

void set_task_state(char *id, char state[]){
    CURL *curl;
    CURLcode res;
    struct string s;
    init_string(&s);
    char host[100] = "http://88.249.152.154:8080/graph/run/";
    strcat(host,id);
    strcat(host,"/");
    strcat(host,state);
    strcat(host,"\0");
    
    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, host);
        curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);
        curl_easy_setopt(curl, CURLOPT_HEADER, 0L);  // Do not include headers in output

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        } else {
            printf("%s\n", s.ptr);
        }

    }
    curl_easy_cleanup(curl);
    free(s.ptr);
}

void worker_execute(int rank, char *buffer, MPI_Status *status){
    while (true) {
            
            MPI_Recv(buffer, 5000, MPI_CHAR, 0, WORK_TAG, MPI_COMM_WORLD, status);
            printf("%d got the data\n", rank);
            sleep(1);
            printf("%d done \n",rank);
            json_t *root;
            json_error_t error;

            root = json_loads(buffer, 0, &error);
            if (!root) {
                fprintf(stderr, "error: on line %d: %s\n", error.line, error.text);
                return;
            }

            char id[50];
            json_t *id_json = json_object_get(root, "id");
            strcpy(id,json_string_value(id_json));
            printf("id: %s \n",id);

            char state[30] = "RUNNING";
            set_task_state(id, state);

            char task[50];
            json_t *task_json = json_object_get(root, "task");
            strcpy(task,json_string_value(task_json));
            printf("task: %s \n",task);

            char inputSource[50];
            json_t *inputSource_json = json_object_get(root, "inputSource");
            strcpy(inputSource,json_string_value(inputSource_json));
            printf("inputSource: %s \n",inputSource);

            char input[50];
            json_t *input_json = json_object_get(root, "input");
            strcpy(input,json_string_value(input_json));
            printf("input: %s \n",input);

            char dataType[50];
            json_t *dataType_json = json_object_get(root, "dataType");
            strcpy(dataType,json_string_value(dataType_json));
            printf("dataType: %s \n",dataType);

            char output[50];
            json_t *output_json = json_object_get(root, "output");
            strcpy(output,json_string_value(output_json));
            printf("output: %s \n",output);


            char runLog[1000];
            int success=0;
            pipeline(task, input, output, dataType, NULL, NULL, inputSource, runLog, &success);
            printf("%d %d pipeline done\n",rank, success);
            if(success==0){
                strcpy(state,"ERROR");
                MPI_Send(NULL, 0, MPI_INT, 0, WORK_DONE_TAG, MPI_COMM_WORLD);
                set_task_state(id, state);
                continue;
            }

            strcpy(state,"TERMINATED");
            MPI_Send(NULL, 0, MPI_INT, 0, WORK_DONE_TAG, MPI_COMM_WORLD);
            set_task_state(id, state);

            json_decref(root);
            json_decref(id_json);
            json_decref(dataType_json);
            json_decref(task_json);
            json_decref(inputSource_json);
            json_decref(input_json);
            json_decref(output_json);

            printf("finished and updated task state\n");
        }
}


void single_execute(){

}

void master_bulk_execute(){

}

void execute_master(int *available, char *buffer, int size, MPI_Request *request, MPI_Status *status){


    for(int q=1;q<16;q++){
        printf("%d ", available[q]);
    }
    printf("\n");

    json_t *root;
    json_error_t error;

    root = json_loads(buffer, 0, &error);
    
    if (!root) {
        fprintf(stderr, "error: on line %d: %s\n", error.line, error.text);
        return;
    }

    //printf("%s\n",buffer);

    char execution_type[20];
    char task_to_execute[1000];
    json_t *executionType_json = json_object_get(root, "executionType");
    strcpy(execution_type,json_string_value(executionType_json));
    //printf("dataType: %s \n",execution_type);

    json_t *task_to_execute_json = json_object_get(root, "task");

    char *jsonString = json_dumps(task_to_execute_json, JSON_INDENT(2));


    //strcpy(task_to_execute,json_string_value(task_to_execute_json));
    //printf("task: %s \n",jsonString);

    if (strcmp(execution_type, "SINGLE") == 0){
        for (int i = 1; i < size; i++) {
        
        if (available[i]) {
            printf("%d find available\n",i);
            pthread_mutex_lock(&mutex);
            printf("maaster lock\n");
            int count = strlen(jsonString) + 1; // Length of the string including null terminator
            MPI_Isend(jsonString, count, MPI_CHAR, i, WORK_TAG, MPI_COMM_WORLD, request);
            available[i] = 0; // Mark worker as busy
            pthread_mutex_unlock(&mutex);
            printf("master unlock\n");
            break;
        }
    }
    }else if (strcmp(execution_type, "BULK") == 0){
        //scheduling algorithm goes here
    }
}

int main(int argc,  char** argv){

    pthread_mutex_init(&mutex, NULL); 

    int rank, size, sock, client_sock;
    struct sockaddr_un addr;
    char buffer[BUFFER_SIZE];
    int available[16] = {0};
    MPI_Status status;
    MPI_Request request;

    int provided;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);

    if (provided < MPI_THREAD_MULTIPLE) {
        printf("Warning: The MPI library does not provide full thread support\n");
    }

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (rank == 0){ //master process listens socket and executes workers
        pthread_t listener_thread;
        //mpi_spawn()
        for (int i = 1; i < size; i++) {
            available[i] = 1;
        }

        if (pthread_create(&listener_thread, NULL, listen_for_availability, available) != 0) {
            perror("Failed to create listener thread");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }


        sock = socket(AF_UNIX, SOCK_STREAM, 0);
        if (sock < 0){
            perror("socket");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);
        unlink(SOCKET_PATH);

        if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0){
            perror("bind");
            close(sock);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        if (listen(sock, 5) < 0){
            perror("listen");
            close(sock);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        printf("MPI Rank 0 listening on UNIX domain socket\n");

        while (1){
            client_sock = accept(sock, NULL, NULL);
            if (client_sock < 0)
            {
                perror("accept");
                continue;
            }

            memset(buffer, 0, BUFFER_SIZE);
            read(client_sock, buffer, BUFFER_SIZE - 1);
            
            execute_master(available, buffer, size, &request, &status);

            close(client_sock);
        }
        pthread_join(listener_thread, NULL);
        close(sock);
    }else{
        printf("%d worker\n",rank);
        worker_execute(rank, buffer, &status);
    }

    MPI_Finalize();
    return 0;
}
