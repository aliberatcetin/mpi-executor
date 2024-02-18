#include "mpi.h"
#include <stdio.h>
#include <cstring>
#include <stdlib.h>
#include <unistd.h>

void free_string_array(char **array, int size){
    for(int i = 0; i < size; i++){
        free(array[i]);
    }
    free(array);
}

/* Example of adding MPI processes */
int main(int argc, char* argv[]){

    printf("GIRDIK\n");
    MPI_Group group = MPI_GROUP_NULL;
    MPI_Session session = MPI_SESSION_NULL;
    MPI_Comm comm = MPI_COMM_NULL;
    MPI_Info info = MPI_INFO_NULL;

    char main_pset[MPI_MAX_PSET_NAME_LEN];
    char boolean_string[16], nprocs[] = "1", **input_psets, **output_psets, host[64];  
    int original_rank, new_rank, flag = 0, dynamic_process = 0, noutput, op;
    int flag_ =0;
    //char data[32];
    gethostname(host, 64);

    printf("%s\n",host);

    char *dict_key = strdup("main_pset");
    
    /* We start with the mpi://WORLD PSet */
    strcpy(main_pset, "mpi://WORLD");

    printf("init2\n");
     /* Initialize the session */
    MPI_Session_init(MPI_INFO_NULL, MPI_ERRORS_ARE_FATAL, &session);

    printf("init\n");
    /* Get the info from our mpi://WORLD pset */
    MPI_Session_get_pset_info (session, main_pset, &info);

    /* get value for the 'mpi_dyn' key -> if true, this process was added dynamically */
    MPI_Info_get(info, "mpi_dyn", 6, boolean_string, &flag);
    printf("%d okuduk? %s\n",flag, boolean_string);   
    MPI_Info_free(&info);

    
    /* if mpi://WORLD is a dynamic PSet retrieve the name of the main PSet stored on mpi://WORLD */
    if(dynamic_process = (flag && 0 == strcmp(boolean_string, "True"))){
 
        /* Lookup the value for the "grown_main_pset" key in the PSet Dictionary and use it as our main PSet */
        MPI_Session_get_pset_data (session, main_pset, main_pset, (char **) &dict_key, 1, true, &info);
        MPI_Info_get(info, "main_pset", MPI_MAX_PSET_NAME_LEN, main_pset, &flag);
        

        MPI_Info_free(&info);


        /*MPI_Session_get_pset_data (session, main_pset, main_pset, (char **) &data_key, 1, true, &info);
        MPI_Info_get(info, "data", 6, data, &flag);
        printf("%d okuduk? %s\n",flag, data);   
        MPI_Info_free(&info);*/

    }
  int number;
    /* create a communcator from our main PSet */
    MPI_Group_from_session_pset (session, main_pset, &group);
    MPI_Comm_create_from_group(group, "mpi.forum.example", MPI_INFO_NULL, MPI_ERRORS_RETURN, &comm);
    MPI_Comm_rank(comm, &original_rank);
    MPI_Group_free(&group);

    printf("ALL Rank %d: Host = '%s', Main PSet = '%s'. I am '%s'! %s \n", original_rank, host, main_pset, dynamic_process ? "dynamic" : "original","");

    /* Original processes will switch to a grown communicator */
    if(!dynamic_process){

        /* One process needs to request the set operation and publish the kickof information */
        while(true){
        if(original_rank == 0){

            
            printf("enter a number: \n");
               
            scanf("%d", &number);
            
            printf("number %d\n", number);
            /* Request the GROW operation */
            op = MPI_PSETOP_ADD;
            
            /* We add nprocs = 2 processes*/
            MPI_Info_create(&info);
            MPI_Info_set(info, "mpi_num_procs_add", nprocs);
    
            /* The main PSet is the input PSet of the operation */
            input_psets = (char **) malloc(1 * sizeof(char*));
            input_psets[0] = strdup(main_pset);
            //strcpy(data, "merhaba");
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
            MPI_Info_set(info, "main_pset", main_pset);
            MPI_Session_set_pset_data(session, output_psets[0], info);

            MPI_Info_free(&info);


            /*MPI_Info_create(&info);
            MPI_Info_set(info, "data", data);
            MPI_Session_set_pset_data(session, output_psets[0], info);
            MPI_Info_free(&info);*/

            free_string_array(input_psets, 1);
            free_string_array(output_psets, noutput);
        }

        /* All processes can query the information about the pending Set operation */      
        MPI_Session_dyn_v2a_query_psetop(session, main_pset, main_pset, &op, &output_psets, &noutput);

        /* Lookup the name of the new main PSet stored on the delta PSet */
        MPI_Session_get_pset_data (session, main_pset, output_psets[0], (char **) &dict_key, 1, true, &info);
        
        MPI_Info_get(info, "main_pset", MPI_MAX_PSET_NAME_LEN, main_pset, &flag); 
        printf("%s o sırada",main_pset);
        free_string_array(output_psets, noutput);
        MPI_Info_free(&info);
        

        
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
    

    /* Disconnect from the old communicator */
    MPI_Comm_disconnect(&comm);

    /* Finalize the MPI Session */
    MPI_Session_finalize(&session);

    printf("im done %d\n", original_rank);
    return 0;
    
}