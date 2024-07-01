#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <jansson.h>
#include <curl/curl.h>
#include <dlfcn.h>
#include "dyn.h"

#include "dyn_psets.h"
#include "dyn_psets_internal.h"

#define PORT 9000 // The port the server will listen on
#define WORK_DONE_TAG 1
#define WORK_TAG 2

int proc_limit = 16;
int step_size = 2;
int iterations = 5;
int num_reconf = 1;
int rm = 0;
int taskMaster = 1;
char mode[64] = "expand";
char step_mode[64] = "DOUBLE";
struct string {
    char *ptr;
    size_t len;
};
reconf_info_t reconf_info = {.cur_iter = 0, .step_size = 2, .is_master=true, .is_dynamic=false, .try=55};

void update_paramters(){
    if (0 == strcmp(step_mode, "DOUBLE")){
        step_size *= 2;
    }else if(0 == strcmp(step_mode, "HALF")){
        step_size /= 2;
    }
}

void error(const char *msg) {
    perror(msg);
    exit(1);
}

int check_limit(int size){
    //printf("LIMIT CHECK %d %d %d\n",size, reconf_info.step_size, proc_limit);
    if(0 == reconf_info.step_size){
        return 0;
    }

    if(0 == strcmp("expand", mode)){
        return size + reconf_info.step_size <= proc_limit;
    }else if (0 == strcmp("shrink", mode)){
        return size - reconf_info.step_size >= proc_limit;
    }
    return 0;
}


int do_work(int num_procs){
  
    //double speedup = (t_p + t_s) / (t_s + t_p / num_procs);
    //double work_duration = base_duration / speedup;
   
    //printf("Sleep for %f\n", work_duration);

   

    return 0;

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
    char host[100] = "http://host.docker.internal:8081/graph/run/";
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




void get_task_detail(char *task_id, char **detail){
    *detail = (char*) malloc(4096);
    CURL *curl;
    CURLcode res;
    struct string s;
    init_string(&s);
    char host[100] = "http://host.docker.internal:8081/graph/executionDetail/";
    strcat(host,task_id);
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
            strcpy(*detail,s.ptr);
        }
    }
    curl_easy_cleanup(curl);
    free(s.ptr);
}

void put_main_pset(char *task_id, char psetname[]){
    CURL *curl;
    CURLcode res;
    struct string s;
    init_string(&s);
    char host[100] = "http://host.docker.internal:8081/key/put/";
    strcat(host,task_id);
    strcat(host,"\0");

    char *post_data = psetname;
    printf("UPDATING %s\n", post_data);
    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, host);
        curl_easy_setopt(curl, CURLOPT_HTTPPOST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);
        curl_easy_setopt(curl, CURLOPT_HEADER, 0L);  // Do not include headers in output

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        } else {
            printf("REQUEST BITTI %s\n", s.ptr);
        }

    }
    curl_easy_cleanup(curl);
    free(s.ptr);
}

void get_main_pset(char *task_id, char **detail){
    *detail = (char*) malloc(4096);
    CURL *curl;
    CURLcode res;
    struct string s;
    init_string(&s);
    char host[100] = "http://host.docker.internal:8081/key/get/";
    strcat(host,task_id);
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
            strcpy(*detail,s.ptr);
        }
    }
    curl_easy_cleanup(curl);
    free(s.ptr);
}

void free_string_array(char **array, int size){
    for(int i = 0; i < size; i++){
        free(array[i]);
    }
    free(array);
}

void execute_manager(json_t *root, fptr *func,int *success){
    

    char id[50];
    json_t *id_json = json_object_get(root, "id");
    strcpy(id,json_string_value(id_json));
    printf("id: %s \n",id);

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


    if (strcmp(inputSource , "file") == 0 && strlen(input)<=3){
        *success = 0;
        return;
    }
    if (strlen(output)<=3){
        *success = 0;
        return;
    }

    //func(task, input, output, dataType, NULL, NULL, inputSource, runLog, &success);

    (*func)(root, input, output, dataType, NULL, 0, success);
    
    // json_decref(dataType_json);
    // json_decref(inputSource_json);
    // json_decref(input_json);
    // json_decref(output_json);
    // json_decref(id_json);
}

void execute(char *data){


    json_t *root;
    json_error_t error;
    printf("hello %s--------------\n---------------\n", data);
    root = json_loads(data, 0, &error);
    if (!root) {
        fprintf(stderr, "error: on line %d: %s\n", error.line, error.text);
        return;
    }

    char id[50];
    json_t *id_json = json_object_get(root, "id");
    strcpy(id,json_string_value(id_json));
    printf("id: %s \n",id);
    char task[50];
    json_t *task_json = json_object_get(root, "task");
    strcpy(task,json_string_value(task_json));
    printf("task: %s \n",task);

    

    fptr func;
    void *handle = dlopen("./libdyn.so", RTLD_NOW);

    if (!handle) {
        fprintf(stderr, "Error: %s\n", dlerror());
        return;
    }

    func = (fptr)dlsym(handle, task);

    if (!func) {
        fprintf(stderr, "Error: %s\n", dlerror());
        dlclose(handle);
        return;
    }

    char state[30] = "RUNNING";
    int success = 1;
    set_task_state(id, state);
    
    execute_manager(root, &func, &success);
    
    printf("pipeline done\n");
    if(success==0){
        strcpy(state,"ERROR");
        set_task_state(id, state);
    }else{
        strcpy(state,"TERMINATED");
        set_task_state(id, state);
    }

    
    json_decref(id_json);
    json_decref(task_json);
    json_decref(root);

}




int expand_send(dyn_pset_state_t *dyn_pset_state){
    printf("expand send %d\n", reconf_info.cur_iter);
    MPI_Bcast(dyn_pset_state->user_pointer,  sizeof(reconf_info), MPI_BYTE, 0, dyn_pset_state->mpicomm);
}

int expand_recv(dyn_pset_state_t *dyn_pset_state){
    MPI_Bcast(dyn_pset_state->user_pointer, sizeof(reconf_info), MPI_BYTE, 0, dyn_pset_state->mpicomm);
    printf("%d receive \n",((reconf_info_t*)dyn_pset_state->user_pointer)->try);
    ++reconf_info.cur_iter;
}

int get_psetop_info(MPI_Info *info){
    char str[256];
    MPI_Info_create(info);                                                                                                
    sprintf(str, "%d", reconf_info.step_size);
    if(rm){
        if(taskMaster){
            printf("TASK MASTER\n");


            char output_space_generator[200];
            sprintf(output_space_generator, "partial(output_space_generator_replace,num_delta_add=%d,num_max=%d,mapping = '1:node',model=ConstantPsetModel,model_params={'speedup':%d})", reconf_info.step_size,proc_limit, num_reconf + 1);
            MPI_Info_set(*info, "model", "DefaultReplaceModel()");
            MPI_Info_set(*info, "output_space_generator", output_space_generator);      
        }else{
            printf("GRAPH MASTER\n");
            char output_space_generator[200];
            sprintf(output_space_generator, "partial(output_space_generator_add,num_delta=1,num_max=32,mapping = '1:node',model=AmdahlPsetModel,model_params={'t_s': 1, 't_p' : 300})");
            MPI_Info_set(*info, "model", "DefaultReplaceModel()");
            MPI_Info_set(*info, "output_space_generator_launch", output_space_generator);      
        }
    }else{
         if(0 == strcmp(mode, "expand")){
            MPI_Info_set(*info, "mpi_num_procs_add", str);
        }else if(0 == strcmp(mode, "shrink")){
            MPI_Info_set(*info, "mpi_num_procs_sub", str);
        }
    }
    return 0;
}


/* Example of adding MPI processes */
int main(int argc, char* argv[]){
    printf("????????????????????MAIN ENTRY????????????????????\n");
    MPI_Group group = MPI_GROUP_NULL;
    MPI_Session session = MPI_SESSION_NULL;
    MPI_Session session2 = MPI_SESSION_NULL;
    MPI_Comm comm = MPI_COMM_NULL;
    MPI_Info info = MPI_INFO_NULL;
    dyn_pset_state_t *dyn_pset_state;
    if (argc != 2) {
        printf("Usage: %s <integer>\n", argv[0]);
        rm = 1;
    }else{
         rm = atoi(argv[1]);
    }
    char main_pset[MPI_MAX_PSET_NAME_LEN];
    char boolean_string[16], nprocs[] = "1", **input_psets, **output_psets, host[64];  
    int original_rank, new_rank, flag = 0, dynamic_process = 0, noutput, op;
    int flag_ =0;
    char *task_id, *data, *task_master;
    task_id = (char*) malloc(64);
    task_master = (char*) malloc(64);
    char *ptype = (char*) malloc(16);
    gethostname(host, 64);


    char *dict_key = strdup("main_pset");
    char *data_key = strdup("data");
    char *task_master_key = strdup("task_master");
    char *ptype_key = strdup("ptype");

    /* We start with the mpi://WORLD PSet */
    strcpy(main_pset, "mpi://WORLD");

   
     /* Initialize the session */
    MPI_Session_init(MPI_INFO_NULL, MPI_ERRORS_ARE_FATAL, &session);

    /* Get the info from our mpi://WORLD pset */
    MPI_Session_get_pset_info (session, main_pset, &info);

    /* get value for the 'mpi_dyn' key -> if true, this process was added dynamically */
    MPI_Info_get(info, "mpi_dyn", 6, boolean_string, &flag);
    //printf("%d? %s\n",flag, boolean_string);
    MPI_Info_free(&info);

    
    if(dynamic_process = (flag && 0 == strcmp(boolean_string, "True"))){

        MPI_Session_get_pset_data (session, main_pset, main_pset, (char **) &ptype_key, 1, true, &info);
        MPI_Info_get(info, ptype_key, 20, ptype, &flag);
        printf("task type info: %d %s \n",flag, ptype);
        MPI_Info_free(&info);
        char *papi = (char*)malloc(64);

        printf("PTYPE: %s \n",ptype);
        if(0 == strcmp(ptype, "master")){
            
            MPI_Session_get_pset_data (session, main_pset, main_pset, (char **) &data_key, 1, true, &info);
            MPI_Info_get(info, "data", 100, task_id, &flag);
            printf("data: %d %s \n",flag, task_id);
            MPI_Info_free(&info);

            get_task_detail(task_id, &data);
            get_main_pset(task_id, &papi);
            printf("DYNAMIC MASTER Rank %d: Host = '%s', Main PSet = '%s'. I am '%s'! %s \n", original_rank, host, papi, dynamic_process ? "dynamic" : "original",task_id);
            execute(data);
        }else{
            reconf_info.is_dynamic=true;
            reconf_info.is_master=false;
            MPI_Session_get_pset_data (session, main_pset, main_pset, (char **) &data_key, 1, true, &info);
            MPI_Info_get(info, "data", 100, task_id, &flag);
            MPI_Info_free(&info);
            get_main_pset(task_id, &papi);
            //printf("@@@@@@@@@@@@@@@@@@@@@@@@@@ MAINPSET: %s\n",papi);
        }


        
        /* Lookup the value for the "grown_main_pset" key in the PSet Dictionary and use it as our main PSet */
        /*MPI_Session_get_pset_data (session, main_pset, main_pset, (char **) &dict_key, 1, true, &info);
        MPI_Info_get(info, "main_pset", MPI_MAX_PSET_NAME_LEN, main_pset, &flag);
        MPI_Info_free(&info);*/

        //printf("ALDIK IŞTE %s \n", main_pset);

        /*
        mpi://world -> spawns -> 0 rank prrte://base_name/0(dynamic)-> dyn_psets(forked) -> (prrte://base_name/3, prrte://base_name/4)
        mpi://world -> spawns -> 0 rank prrte://base_name/1(dynamic) -> dyn_psets(forked) ->  prrte://base_name/5(dynamic)
        */

/*
    prrte://base_name/0(dynamic) -> mpirun --np dyn.c --host --parameters
*/  


       
        


        /*MPI_Session_get_pset_data (session, main_pset, main_pset, (char **) &task_master_key, 1, true, &info);
        MPI_Info_get(info, task_master_key, 100, task_master, &flag);
        printf("master info: %d %s \n",flag, task_master);
        MPI_Info_free(&info);
        printf("before init\n");
        
        MPI_Group_from_session_pset (session, main_pset, &group);
        MPI_Comm_create_from_group(group, "mpi.forum.example", MPI_INFO_NULL, MPI_ERRORS_RETURN, &comm);
        MPI_Comm_rank(comm, &original_rank);
        MPI_Group_free(&group);*/
        int terminate, rc, reconfigured;

        MPI_Info info2;
        if(0 != get_psetop_info(&info2)){
            printf("ERROR CREATING INFO. TERMINATE!");
            exit(-1);
        }

        dyn_pset_state = dyn_pset_init(task_id, session, main_pset, &reconf_info, MPI_INFO_NULL, expand_send, expand_recv, NULL, NULL);
        //user_func(dyn_pset_state, main_pset, task_id)
        //finalize();
        
    

        //printf("AFTER INIT\n");
		
        if(0 != dyn_pset_set_info(dyn_pset_state, info2)){
            printf("ERROR IN DYN_PSET_SET_INFO. TERMINATE!");
            exit(-1);
        }
    
        //printf("after init %d\n", dyn_pset_state == NULL);
        /*if( ((dyn_pset_data_t*)(dyn_pset_state->dyn_pset_data))->info == MPI_INFO_NULL){
        printf("ALDIK NULL OLDU MASTERDA\n");
        }
        printf("%s after init\n",((dyn_pset_data_t*)(dyn_pset_state->dyn_pset_data))->main_pset);*/
        /*if(strcmp(task_master,"True")==0){
            printf("-----------MASTERIM BEN----------\n");
            if(0 != dyn_pset_adapt(dyn_pset_state, &terminate, &reconfigured)){
                printf("ERROR IN DYN_PSET_ADAPT. TERMINATE!");
                exit(-1);          
            }
        }*/
    
       
    MPI_Barrier(dyn_pset_state->mpicomm);
    /* ======= START MAIN LOOP ======= */
    for(; reconf_info.cur_iter < iterations; reconf_info.cur_iter++){
        
        if (dyn_pset_state->mpirank == 0){
            reconf_info.try = 67;
            printf("Rank %d / %d: Starting iter %d / %d\n", dyn_pset_state->mpirank, dyn_pset_state->mpisize, reconf_info.cur_iter+1, iterations);
        }else{
            printf("DYNAMIC Rank %d / %d: Starting iter %d / %d\n", dyn_pset_state->mpirank, dyn_pset_state->mpisize, reconf_info.cur_iter+1, iterations);
        }

        do_work(dyn_pset_state->mpisize);
        //printf("FIRST BARRIER BEFORE FIRST BARRIER  %d\n",dyn_pset_state->mpirank);
        MPI_Barrier(dyn_pset_state->mpicomm);
        //printf("FIRST BARRIER AFTER FIRST BARRIER  %d\n",dyn_pset_state->mpirank);

        if (check_limit(dyn_pset_state->mpisize)){
            printf("limit ok %d\n", dyn_pset_state->mpirank);
            if(0 != dyn_pset_adapt(dyn_pset_state, &terminate, &reconfigured)){
                printf("ERROR IN DYN_PSET_ADAPT. TERMINATE!");
                exit(-1);
            }
        }else{
             //printf("limit NOT ok %d\n", dyn_pset_state->mpirank);
        }
        int num_reconf = 1;
        if(terminate){
            printf("TERMINATE %d\n",dyn_pset_state->mpirank);
            //break;
        }
        //printf("BEFORE SECOND BARRIER  %d\n",dyn_pset_state->mpirank);
        MPI_Barrier(dyn_pset_state->mpicomm);
        //printf("AFTER SECOND BARRIER %d\n",dyn_pset_state->mpirank);
        if(reconfigured){
            num_reconf++;
	        update_paramters();
            MPI_Info_free(&info2);
            if(0 != get_psetop_info(&info2)){
                printf("ERROR CREATING INFO. TERMINATE!");
                exit(-1);
            }
            if(0 != dyn_pset_set_info(dyn_pset_state, info2)){
                printf("ERROR IN DYN_PSET_SET_INFO. TERMINATE!");
                exit(-1);           
            }
            
        }


    }
    
    printf("END \n");
    //free(data);
    }else{
        MPI_Group_from_session_pset (session, main_pset, &group);
        MPI_Comm_create_from_group(group, "mpi.forum.example", MPI_INFO_NULL, MPI_ERRORS_RETURN, &comm);
        MPI_Comm_rank(comm, &original_rank);
        MPI_Group_free(&group);
        MPI_Comm_disconnect(&comm);
        printf("ALL Rank %d: Host = '%s', Main PSet = '%s'. I am '%s'! %s \n", original_rank, host, main_pset, dynamic_process ? "dynamic" : "original",task_id);
    }
    int number;
    /* create a communcator from our main PSet */
    
    free(task_id);
    /* Original processes will switch to a grown communicator */
    if(!dynamic_process){
        taskMaster = 0;
        int sockfd, newsockfd;
        socklen_t clilen;
        char *buffer;
        buffer = (char*)malloc(1024);
        struct sockaddr_in serv_addr, cli_addr;
        int n;

        // Create a socket
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            error("ERROR opening socket");
        }

        // Clear address structure
        memset(&serv_addr, 0, sizeof(serv_addr));

        // Setup the host_addr structure for use in bind call
        serv_addr.sin_family = AF_INET;  
        serv_addr.sin_addr.s_addr = INADDR_ANY; // Automatically be filled with current host's IP address
        serv_addr.sin_port = htons(PORT);

        // Bind the socket to the current IP address on port, PORT
        if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
            error("ERROR on binding");
        }

        // Listen for incoming connections
        listen(sockfd, 5);
        clilen = sizeof(cli_addr);
        char t[50];
        strcpy(t,"True");
        char ptype[10];
        strcpy(ptype,"master");
        // prrtee://base_anme/0-> master
    /* One process needs to request the set operation and publish the kickof information */
        while(true){
        if(original_rank == 0){
            printf("Waiting for a connection...\n");

            // Accept an incoming connection
            newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
            if (newsockfd < 0) {
                error("ERROR on accept");
            }

            printf("Connection accepted.\n");

            // Clear the buffer with zeros
            memset(buffer, 0, 3000);

            // Read the message from the client
            n = read(newsockfd, buffer, 2999);
            if (n < 0) {
                error("ERROR reading from socket");
            }


            json_t *root;
            json_error_t e;

            root = json_loads(buffer, 0, &e);
            char id[50];
            if (!root) {
                fprintf(stderr, "error: on line %d: %s\n", e.line, e.text);
            }else{
                json_t *id_json = json_object_get(root, "id");
                strcpy(id,json_string_value(id_json));
                printf("id: %s \n",id);
                json_decref(root);
                json_decref(id_json);
            }
            //strcat(data,"\0");
            printf("Here is the message: %s\n", buffer);

            // Respond to client
            n = write(newsockfd, "I got your message", 18);
            if (n < 0) {
                error("ERROR writing to socket");
            }

            // Close the connection socket
            close(newsockfd);

            /* Request the GROW operation */
            op = MPI_PSETOP_ADD;
            
            /* We add nprocs = 2 processes*/
            
            MPI_Info_create(&info);
            if(rm){
                printf("GRAPH MASTER\n");
                char output_space_generator[200];
                sprintf(output_space_generator, "partial(output_space_generator_launch,num_delta=1,num_max=32,mapping = '1:node',model=AmdahlPsetModel,model_params={'t_s': 1, 't_p' : 300})");
                MPI_Info_set(info, "model", "DefaultAddModel()");
                MPI_Info_set(info, "output_space_generator", output_space_generator);      
            }else{
                MPI_Info_set(info, "mpi_num_procs_add", nprocs);
            }
            
            
    
            /* The main PSet is the input PSet of the operation */
            input_psets = (char **) malloc(1 * sizeof(char*));
            input_psets[0] = strdup(main_pset);
            
            noutput = 0;
            /* Send the Set Operation request */
            printf("before set op\n");
            MPI_Session_dyn_v2a_psetop(session, &op, input_psets, 1, &output_psets, &noutput, info);
            printf("noutput size %d\n", noutput);
            for(int i=0;i<noutput;i++){
                printf("output :%s\n",output_psets[i]);
            }
            MPI_Info_free(&info);


            /* Publish the name of the new main PSet on the delta Pset */
            
            MPI_Info_create(&info);
            MPI_Info_set(info, "main_pset", output_psets[0]);
            MPI_Session_set_pset_data(session, output_psets[0], info);
            MPI_Info_free(&info);

            MPI_Info_create(&info);
            MPI_Info_set(info, "data", id);
            MPI_Session_set_pset_data(session, output_psets[0], info);
            MPI_Info_free(&info);

            
            put_main_pset(id, output_psets[0]);

            
            MPI_Info_create(&info);
            MPI_Info_set(info, task_master_key, t);
            MPI_Session_set_pset_data(session, output_psets[0], info);
            MPI_Info_free(&info);


            MPI_Info_create(&info);
            MPI_Info_set(info, ptype_key, ptype);
            MPI_Session_set_pset_data(session, output_psets[0], info);
            MPI_Info_free(&info);

            free_string_array(input_psets, 1);
            free_string_array(output_psets, noutput);
            
        }

        /* All processes can query the information about the pending Set operation */      
        //MPI_Session_dyn_v2a_query_psetop(session, main_pset, main_pset, &op, &output_psets, &noutput);

        /* Lookup the name of the new main PSet stored on the delta PSet */
        //MPI_Session_get_pset_data (session, main_pset, output_psets[0], (char **) &dict_key, 1, true, &info);
        
        //MPI_Info_get(info, "main_pset", MPI_MAX_PSET_NAME_LEN, main_pset, &flag); 
        //printf("%s o sırada",main_pset);
        //free_string_array(output_psets, noutput);
        //MPI_Info_free(&info);
        

        
        /* Disconnect from the old communicator */
        if(!flag_){
            printf("not null\n");
            //MPI_Comm_disconnect(&comm);
            flag_=1;
        }else{
            printf("null\n");
        }
        
    
        /* create a cnew ommunicator from the new main PSet*/
        //MPI_Group_from_session_pset (session, main_pset, &group);
        //MPI_Comm_create_from_group(group, "mpi.forum.example", MPI_INFO_NULL, MPI_ERRORS_RETURN, &comm);
        
        //MPI_Bcast(&number, 1, MPI_INT, 0, comm);                                                    
        

        //MPI_Send(&number, 1, MPI_INT, 1, 0, comm);
        //MPI_Comm_rank(comm, &new_rank);
        //MPI_Comm_disconnect(&comm);
        //MPI_Group_free(&group);

        /* Indicate completion of the Pset operation*/
        if(original_rank == 0){
            MPI_Session_dyn_finalize_psetop(session, main_pset);
        }

        printf("STATIC Rank %d: Host = '%s', Main PSet = '%s'. I am 'original'!\n", new_rank, host, main_pset);
        
       

        }
    }else{
        printf("im dynamic\n");
        

        ///MPI_Bcast(&number, 1, MPI_INT, 0, comm);       
        //prntf("Process %d got number %d \n", original_rank, number);
    }
     printf("im done %d %d master: %d dynamic: %d\n", original_rank, dynamic_process, dyn_pset_state->is_master, dyn_pset_state->is_dynamic);
    if(0 != dyn_pset_finalize(&dyn_pset_state, NULL)){
        printf("ERROR IN DYN_PSET_FINALIZE. TERMINATE!");
        exit(-1);
    }
    /* Disconnect from the old communicator */
    //MPI_Comm_disconnect(&comm);
    MPI_Session_finalize(&session);
    /* Finalize the MPI Session */
  

   
    return 0;
    
}