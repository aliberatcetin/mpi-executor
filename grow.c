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

    MPI_Group group = MPI_GROUP_NULL;
    MPI_Session session = MPI_SESSION_NULL;
    MPI_Comm comm = MPI_COMM_NULL;
    MPI_Info info = MPI_INFO_NULL;

    char main_pset[MPI_MAX_PSET_NAME_LEN];
    char boolean_string[16], nprocs[] = "2", **input_psets, **output_psets, host[64];  
    int original_rank, new_rank, flag = 0, dynamic_process = 0, noutput, op;

    gethostname(host, 64);

    char *dict_key = strdup("main_pset");

    /* We start with the mpi://WORLD PSet */
    strcpy(main_pset, "mpi://WORLD");

     /* Initialize the session */
    MPI_Session_init(MPI_INFO_NULL, MPI_ERRORS_ARE_FATAL, &session);

    /* Get the info from our mpi://WORLD pset */
    MPI_Session_get_pset_info (session, main_pset, &info);

    /* get value for the 'mpi_dyn' key -> if true, this process was added dynamically */
    MPI_Info_get(info, "mpi_dyn", 6, boolean_string, &flag);
    MPI_Info_free(&info);

    /* if mpi://WORLD is a dynamic PSet retrieve the name of the main PSet stored on mpi://WORLD */
    if(dynamic_process = (flag && 0 == strcmp(boolean_string, "True"))){

        /* Lookup the value for the "grown_main_pset" key in the PSet Dictionary and use it as our main PSet */
        MPI_Session_get_pset_data (session, main_pset, main_pset, (char **) &dict_key, 1, true, &info);
        MPI_Info_get(info, "main_pset", MPI_MAX_PSET_NAME_LEN, main_pset, &flag);
        MPI_Info_free(&info);
    }

    /* create a communcator from our main PSet */
    MPI_Group_from_session_pset (session, main_pset, &group);
    MPI_Comm_create_from_group(group, "mpi.forum.example", MPI_INFO_NULL, MPI_ERRORS_RETURN, &comm);
    MPI_Comm_rank(comm, &original_rank);
    MPI_Group_free(&group);

    printf("Rank %d: Host = '%s', Main PSet = '%s'. I am '%s'!\n", original_rank, host, main_pset, dynamic_process ? "dynamic" : "original");

    /* Original processes will switch to a grown communicator */
    if(!dynamic_process){

        /* One process needs to request the set operation and publish the kickof information */
        if(original_rank == 0){

            /* Request the GROW operation */
            op = MPI_PSETOP_GROW;

            /* We add nprocs = 2 processes*/
            MPI_Info_create(&info);
            MPI_Info_set(info, "mpi_num_procs_add", nprocs);
    
            /* The main PSet is the input PSet of the operation */
            input_psets = (char **) malloc(1 * sizeof(char*));
            input_psets[0] = strdup(main_pset);

            noutput = 0;
            /* Send the Set Operation request */
            MPI_Session_dyn_v2a_psetop(session, &op, input_psets, 1, &output_psets, &noutput, info);
            MPI_Info_free(&info);

            /* Publish the name of the new main PSet on the delta Pset */
            MPI_Info_create(&info);
            MPI_Info_set(info, "main_pset", output_psets[1]);
            MPI_Session_set_pset_data(session, output_psets[0], info);
            MPI_Info_free(&info);
            free_string_array(input_psets, 1);
            free_string_array(output_psets, noutput);
        }
        /* All processes can query the information about the pending Set operation */      
        MPI_Session_dyn_v2a_query_psetop(session, main_pset, main_pset, &op, &output_psets, &noutput);

        /* Lookup the name of the new main PSet stored on the delta PSet */
        MPI_Session_get_pset_data (session, main_pset, output_psets[0], (char **) &dict_key, 1, true, &info);
        MPI_Info_get(info, "main_pset", MPI_MAX_PSET_NAME_LEN, main_pset, &flag); 
        free_string_array(output_psets, noutput);
        MPI_Info_free(&info);
    
        /* Disconnect from the old communicator */
        MPI_Comm_disconnect(&comm);
    
        /* create a cnew ommunicator from the new main PSet*/
        MPI_Group_from_session_pset (session, main_pset, &group);
        MPI_Comm_create_from_group(group, "mpi.forum.example", MPI_INFO_NULL, MPI_ERRORS_RETURN, &comm);
        MPI_Comm_rank(comm, &new_rank);
        MPI_Group_free(&group);

        /* Indicate completion of the Pset operation*/
        if(original_rank == 0){
            MPI_Session_dyn_finalize_psetop(session, main_pset);
        }

        printf("STATIC OLAN Rank %d: Host = '%s', Main PSet = '%s'. I am 'original'!\n", new_rank, host, main_pset);
        
    }
    
    /* Disconnect from the old communicator */
    MPI_Comm_disconnect(&comm);

    /* Finalize the MPI Session */
    MPI_Session_finalize(&session);

    return 0;
    
}


