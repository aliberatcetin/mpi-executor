#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include "dyn_psets.h"
#include "dyn_psets_internal.h"



dyn_pset_data_t * dyn_pset_data_new(MPI_Session session, char *pset,reconf_info_t *info_, dyn_psets_expand_send_func_t exp_send, dyn_psets_expand_recv_func_t exp_recv, dyn_psets_shrink_send_func_t shr_send, dyn_psets_shrink_recv_func_t shr_recv){
    int flag = 0;
    char boolean_string[10];
    char task_master[10];
    char *key = "dyn_pset";
    MPI_Info info;
    

    dyn_pset_data_t *data = (dyn_pset_data_t *) malloc(sizeof(dyn_pset_data_t));
    data->session = MPI_SESSION_NULL;
    data->info = MPI_INFO_NULL;
    data->request = MPI_REQUEST_NULL;
    data->setop_pending = false;
    data->is_master = info_->is_master;
    data->is_dynamic = info_->is_dynamic;
    data->output_psets = NULL;
    data->noutput_psets = 0;
    data->exp_send = exp_send;
    data->exp_recv = exp_recv;
    data->shr_send = shr_send;
    data->shr_recv = shr_recv;
    int primary = 0;
    
    MPI_Session_init(MPI_INFO_NULL, MPI_ERRORS_ARE_FATAL, &data->session);

    /* Get the info from our mpi://WORLD pset */
    strcpy(data->main_pset, pset);
    //printf("-----mainpset---- %s\n",pset);
    MPI_Session_get_pset_info (data->session, data->main_pset, &info);

    MPI_Info_get(info, "mpi_primary", 6, boolean_string, &flag);
    MPI_Info_get(info, "task_master", 10, task_master, &flag);
    primary = 0;
    if(0 != strcmp(boolean_string, "True")){
        //printf("-----------PRIMARY----------------\n");
        primary = 1;
    }else{
        //printf("-----------NOT PRIMARY----------------\n");
    }
    /* get value for the 'mpi_dyn' key -> if true, this process was added dynamically */
    MPI_Info_get(info, "mpi_dyn", 6, boolean_string, &flag);

    /* if mpi://WORLD is a dynamic PSet retrieve the name of the main PSet stored on mpi://WORLD */
    if(!data->is_master && flag && 0 == strcmp(boolean_string, "True")){
        MPI_Info_free(&info);
        //printf("-----------NOT MASTER----------------\n");
        data->is_dynamic = true;
        data->setop_pending = true;
        MPI_Session_get_pset_data (data->session, data->main_pset, data->main_pset, &key, 1, true, &info);
        MPI_Info_get(info, key, MPI_MAX_PSET_NAME_LEN, data->main_pset, &flag);

        if(!flag){
            printf("No 'next_main_pset' was provided for dynamic process. Terminate.\n");
            dyn_pset_data_free(&data);
            return NULL;
        }
        MPI_Info_free(&info);

        /* This process was spawned but the simulation is about to end so this process needs to terminate as well */
        if(0 == strcmp(data->main_pset, "dyn_psets://canceled")){
            if(primary){
                MPI_Info_create(&info);                                                                             
                MPI_Info_set(info, "dyn_psets://canceled", "dyn_psets://canceled/ack");                                                  
                MPI_Session_set_pset_data(data->session, "mpi://WORLD", info);
                MPI_Info_free(&info); 
            }
            dyn_pset_data_free(&data);
            return NULL;
        }

    }else{
        //printf("-----------MASTER----------------\n");
        data->is_dynamic = false;
        strcpy(data->main_pset, pset);
    }

    fflush(NULL);
    return data;
}

void dyn_pset_data_free(dyn_pset_data_t ** data_ptr){
    dyn_pset_data_t *data = *data_ptr;
    if (MPI_INFO_NULL != data->info){
      MPI_Info_free(&data->info);
    }
    if (MPI_REQUEST_NULL != data->request){
      MPI_Request_free(&data->request);
    }
    if (MPI_SESSION_NULL != data->session){
      MPI_Session_finalize(&data->session);
    }
    if(NULL != data->output_psets){
        free_string_array(&data->output_psets, data->noutput_psets);
    }
    free(data);
}

dyn_pset_state_t * dyn_pset_state_new(MPI_Session session, char *pset, void * user_pointer,
                            dyn_psets_expand_send_func_t exp_send, dyn_psets_expand_recv_func_t exp_recv,
                            dyn_psets_shrink_send_func_t shr_send, dyn_psets_shrink_recv_func_t shr_recv){
    dyn_pset_state_t * state = malloc(sizeof(dyn_pset_state_t));
    state->mpirank = 0;
    state->mpisize = 0;
    state->is_dynamic = 0;
    reconf_info_t *t = (reconf_info_t*)user_pointer;
    printf("%d %d state new\n", t->is_master, t->is_dynamic);
    state->is_master = t->is_master;
    state->is_dynamic = t->is_dynamic;
    state->mpicomm = MPI_COMM_NULL;
   
    state->dyn_pset_data = (void *) dyn_pset_data_new(session, pset,t, exp_send, exp_recv, shr_send, shr_recv);

    if(NULL == state->dyn_pset_data){
        printf("==============NULL STATE ==========\n");
        dyn_pset_state_free(&state);
        return NULL;
    }
    state->user_pointer = user_pointer;
    state->is_dynamic = ((dyn_pset_data_t *)state->dyn_pset_data)->is_dynamic;
    state->is_master = ((dyn_pset_data_t *)state->dyn_pset_data)->is_master;
    return state;
}
void dyn_pset_state_free(dyn_pset_state_t **state);
void dyn_pset_state_free(dyn_pset_state_t **state){
   
    if(MPI_COMM_NULL != (*state)->mpicomm){
        MPI_Comm_disconnect(&(*state)->mpicomm);
    }
    if(NULL != (*state)->dyn_pset_data){
        dyn_pset_data_free((dyn_pset_data_t **) &(*state)->dyn_pset_data);
    }
    free(*state);
}



void free_string_array(char ***array, int size){
    int i;
    if(0 == size){
        *array = NULL;
        return;
    }
    for(i = 0; i < size; i++){
        free((*array)[i]);
    }
    
    free(*array);

    *array = NULL;
}


void send_setop(dyn_pset_state_t * state){

    char ** input_psets;
    MPI_Info info;
    dyn_pset_data_t * data = (dyn_pset_data_t *) state->dyn_pset_data;
                                                             
    input_psets = (char **)malloc(1 * sizeof(char *));                                                                                 
    input_psets[0] = strdup(data->main_pset);
    data->setop = MPI_PSETOP_REPLACE;
    printf("%s girdik\n",data->main_pset);
    MPI_Session_dyn_v2a_psetop(data->session, &data->setop, input_psets, 1, &data->output_psets, &data->noutput_psets, data->info);                   
    printf("noutput size %d %d %d\n", data->noutput_psets, data->is_master, data->is_dynamic);
    char ptype[10];
    strcpy(ptype, "task");
    char t[50];
    strcpy(t,"False");
    for(int i=0;i<data->noutput_psets;i++){
        printf("REPLACE output :%s %s\n",data->output_psets[i], data->main_pset);
        
        MPI_Info_create(&info);
        MPI_Info_set(info, "ptype", ptype);
        MPI_Session_set_pset_data(data->session, data->output_psets[i], info);
        MPI_Info_free(&info);

        MPI_Info_create(&info);
        MPI_Info_set(info, "data", data->task_id);
        MPI_Session_set_pset_data(data->session, data->output_psets[i], info);
        MPI_Info_free(&info);

        MPI_Info_create(&info);
        MPI_Info_set(info, "task_master", t);
        MPI_Session_set_pset_data(data->session, data->output_psets[i], info);
        MPI_Info_free(&info);

    }
    //free_string_array(&input_psets, 1);
    data->setop_pending = true;
}

void check_setop(dyn_pset_state_t * state, int blocking, int * flag){
    MPI_Status status;
    MPI_Info info;
    dyn_pset_data_t * data = (dyn_pset_data_t *) state->dyn_pset_data;

    if(blocking){
        MPI_Wait(&data->request, MPI_STATUS_IGNORE);
        *flag = 1;
    }else{
        MPI_Test(&data->request, flag, &status);
    }

    if(*flag){
        if(data->setop == MPI_PSETOP_REPLACE){
            data->request = MPI_REQUEST_NULL;
            strcpy(data->delta_sub, data->output_psets[0]);
            strcpy(data->delta_add, data->output_psets[1]);
            strcpy(data->main_pset, data->output_psets[2]);
            //free_string_array(&data->output_psets, data->noutput_psets);

            MPI_Info_create(&info);                                                                             
            MPI_Info_set(info, "dyn_pset", data->main_pset);                                                  
            MPI_Session_set_pset_data(data->session, data->delta_add, info);
            MPI_Info_free(&info);
            
            data->noutput_psets = 0;
        }else{
            /* Try again */
            send_setop(state);
            *flag = 0;
        }
    }
}

void handle_setop(dyn_pset_state_t * state, int * terminate, int *reconfigured){

    int flag;
    char boolean_string[10];
    MPI_Group group;
    MPI_Info info;

    dyn_pset_data_t * data = (dyn_pset_data_t *) state->dyn_pset_data;

    *reconfigured = 1;
    data->setop_pending = false;

    /* Distribute the new pset names */
    printf("HANDLE BCAST enter \n");
    MPI_Bcast(data->delta_add, MPI_MAX_PSET_NAME_LEN, MPI_CHAR, 0, state->mpicomm);
    MPI_Bcast(data->delta_sub, MPI_MAX_PSET_NAME_LEN, MPI_CHAR, 0, state->mpicomm);
    MPI_Bcast(data->main_pset, MPI_MAX_PSET_NAME_LEN, MPI_CHAR, 0, state->mpicomm);
    printf("HANDLE BCAST exit %s\n",data->delta_sub);
    /* Check if this process is included in the new PSet */                                        
    MPI_Session_get_pset_info(data->session, data->main_pset, &info);                                                                            
    MPI_Info_get(info, "mpi_included", 6, boolean_string, &flag);                                                                      
    MPI_Info_free(&info);

    if(0 != strcmp("", data->delta_sub)){
        if(0 != strcmp(boolean_string, "True")){

            /* Send away our data before we leave */
            if(NULL != data->shr_send){
                data->shr_send(state);
            }
            printf("GIRDIM 1 YAPCAM\n");
            *terminate = 1;
            state->mpirank = -1;
            data->rank = -1;
            return;
        }
        if(NULL != data->shr_recv){
            data->shr_recv(state);
        }
    }
    MPI_Comm_disconnect(&state->mpicomm);

    /* Create a new comm */
    MPI_Group_from_session_pset (data->session, data->main_pset, &group);
    MPI_Comm_create_from_group(group, "mpi.forum.example", MPI_INFO_NULL, MPI_ERRORS_RETURN, &state->mpicomm);
    MPI_Group_free(&group);

    /* Store new mpi rank */
    MPI_Comm_rank(state->mpicomm, &data->rank);
    state->mpirank = data->rank;

    /* Store the new size */
    MPI_Comm_size(state->mpicomm, &state->mpisize);

    if(0 != strcmp("", data->delta_add)){
        /* Pack and send the user data */
        if(NULL != data->exp_send){
            data->exp_send(state);
        }
    }

    /* Rank 0 of new comm finalizes the set operation*/
    if(data->rank == 0){
        MPI_Session_dyn_finalize_psetop(data->session, data->main_pset);
    }
}

dyn_pset_state_t * dyn_pset_init(char *task_id, MPI_Session session, const char *initial_pset, void *user_pointer, MPI_Info info,
                    dyn_psets_expand_send_func_t exp_send, dyn_psets_expand_recv_func_t exp_recv,
                    dyn_psets_shrink_send_func_t shr_send, dyn_psets_shrink_recv_func_t shr_recv){
                    
    MPI_Group group = MPI_GROUP_NULL;
    //flag for dynmamic process
    
    dyn_pset_state_t *dyn_pset_state = dyn_pset_state_new(session, initial_pset, user_pointer, exp_send, exp_recv, shr_send, shr_recv);
    
    if(NULL == dyn_pset_state){
        return NULL;
    }
    dyn_pset_data_t * dyn_pset_data = (dyn_pset_data_t *) dyn_pset_state->dyn_pset_data;
    dyn_pset_data->task_id = (char*) malloc(64);
    strcpy(dyn_pset_data->task_id, task_id);
    
    if(dyn_pset_data->info == NULL){
        printf("INFO NULL\n");
    }
    if(MPI_INFO_NULL != info){
        dyn_pset_set_info(dyn_pset_state, info);
    }
    /* create a group from pset */
    int result = MPI_Group_from_session_pset(dyn_pset_data->session, dyn_pset_data->main_pset, &group);
    if (result != MPI_SUCCESS) {
        char error_string[BUFSIZ];
        int length_of_error_string, error_class;
        MPI_Error_class(result, &error_class);
        MPI_Error_string(error_class, error_string, &length_of_error_string);
        printf("Error creating group from session pset: %s\n", error_string);
    } else {
        //printf("SUCCESSS GROUP %s\n", dyn_pset_data->main_pset);
    }


    if (group == MPI_GROUP_NULL) {
        printf("Group is MPI_GROUP_NULL, cannot create communicator.\n");
    } else {
        // Create a communicator from group
        result = MPI_Comm_create_from_group(group, "mpi.forum.example", MPI_INFO_NULL, MPI_ERRORS_RETURN, &dyn_pset_state->mpicomm);
        if (dyn_pset_state->mpicomm == MPI_COMM_NULL) {
            printf("COMM NULL %s\n", dyn_pset_data->main_pset);
            if (result != MPI_SUCCESS) {
                char error_string[BUFSIZ];
                int length_of_error_string, error_class;
                MPI_Error_class(result, &error_class);
                MPI_Error_string(error_class, error_string, &length_of_error_string);
                printf("Error creating communicator from group: %s\n", error_string);
            }else{
                 //printf("SUCCESSS COMM %s\n", dyn_pset_data->main_pset);
            }
        } else {
            //printf("NOT NULL COMM %s\n", dyn_pset_data->main_pset);
        }
    }
   

    /* create a communicator from group */
    /*MPI_Comm_create_from_group(group, "mpi.forum.example", MPI_INFO_NULL, MPI_ERRORS_RETURN, &dyn_pset_state->mpicomm);
    if(dyn_pset_state->mpicomm == MPI_COMM_NULL){
        printf("COMM NULL %s\n", dyn_pset_data->main_pset);
    }else{
        printf("NOT NULL COMM %s\n",dyn_pset_data->main_pset);
    }*/
    MPI_Group_free(&group);
    /* Store our rank */
    MPI_Comm_rank(dyn_pset_state->mpicomm, &dyn_pset_data->rank);

    dyn_pset_state->mpirank = dyn_pset_data->rank;

    /* Store our size */
    MPI_Comm_size(dyn_pset_state->mpicomm, &dyn_pset_state->mpisize);

    if(dyn_pset_state->is_dynamic){
        /* Recv data */
        if(NULL != dyn_pset_data->exp_recv){
            dyn_pset_data->exp_recv(dyn_pset_state);
        }
    }

    /* If this was the launch of the original processes we send a set operation right away  
     * If this was a dynamic launch we might need to finalize the set operation
     */
    if(!dyn_pset_state->is_dynamic && dyn_pset_data->is_master){

        /* only send a setop if the user provided an info object */
        if(dyn_pset_data->info != MPI_INFO_NULL){
            dyn_pset_data->setop_pending = true;
            if(dyn_pset_data->rank == 0){
                send_setop(dyn_pset_state);
            }
        }

    }else{
        dyn_pset_data->setop_pending = true;
        if(dyn_pset_data->rank == 0){
            MPI_Session_dyn_finalize_psetop(dyn_pset_data->session, dyn_pset_data->main_pset);
        }        
    }

    return dyn_pset_state;

}

int dyn_pset_adapt(dyn_pset_state_t * state, int *terminate, int *reconfigured){
    int flag;

    dyn_pset_data_t *dyn_pset_data = (dyn_pset_data_t *) state->dyn_pset_data;
    
    printf("%s after init\n",((dyn_pset_data_t*)(state->dyn_pset_data))->main_pset);


    *terminate = 0;
    *reconfigured = 0;
    
    if(!dyn_pset_data->setop_pending){
        //printf("SENDING SETOP1\n");          
        if(dyn_pset_data->info != MPI_INFO_NULL){     
              //printf("SENDING SETOP2\n");                                                                                                                                     
            if (dyn_pset_data->rank == 0)                                                                                    
            {
                //printf("SENDING SETOP3\n");                                                                                                                       
                send_setop(state);                                                                                                         
            }
            dyn_pset_data->setop_pending = true;
        }
    }
    //printf("SETOP ÇIKIŞ\n");

    if(dyn_pset_data->setop_pending){
        /* Rank 0 requested the setop, so it does the check and Bcasts the result */
        if(dyn_pset_data->rank == 0){
            check_setop(state, 1, &flag);
        }
        
        //printf("BCAST ENTER %d\n",flag);
        MPI_Bcast(&flag, 1, MPI_INT, 0, state->mpicomm);
        //printf("BCAST EXIT %d\n",flag);
        /* Setop still pending, so bail out here */
        if(!flag){
            return 0;
        }
        handle_setop(state, terminate, reconfigured);
        if(*terminate){
            return 0;
        }
    }

    return 0; 
}

int dyn_pset_adapt_nb(dyn_pset_state_t * state, int *terminate, int *reconfigured){
    int flag = 0;

    dyn_pset_data_t *dyn_pset_data = (dyn_pset_data_t *) state->dyn_pset_data;

    *terminate = 0;
    *reconfigured = 0;
    
    /* If we have a pending setop we need to check if it was executed 
     * and if so adapt accordingly
     */
    if(dyn_pset_data->setop_pending){
        /* Rank 0 requested the setop, so it does the check and Bcasts the result */
        if(dyn_pset_data->rank == 0){
            check_setop(state, 0, &flag);
        }
        MPI_Bcast(&flag, 1, MPI_INT, 0, state->mpicomm);
        /* Setop still pending, so bail out here */
        if(!flag){
            return 0;
        }

        handle_setop(state, terminate, reconfigured);

        if(*terminate){
            return 0;
        }
    }
    if(!dyn_pset_data->setop_pending){                                                                                                                                
        if (dyn_pset_data->rank == 0)                                                                                    
        {                                                                                                                                      
            send_setop(state);                                                                                                         
        }

        dyn_pset_data->setop_pending = true;
    } 

    return 0;

}                                                                                                                                      
     
int dyn_pset_finalize(dyn_pset_state_t ** state, char *final_pset){

    char **input_psets, **output_psets;
    char *key = "dyn_psets://canceled";
    char ack_response[MPI_MAX_PSET_NAME_LEN]; 
    int flag, setop, noutput = 0;
    MPI_Info info;
    dyn_pset_state_t *dyn_pset_state = *state;
    dyn_pset_data_t *data = dyn_pset_state->dyn_pset_data;
    MPI_Session_finalize(data->session);
    if(NULL != final_pset){
        strcpy(final_pset, data->main_pset);
    }

    if (data->setop_pending){
        if(data->rank == 0){
            /* Send the setop cancelation - op will be MPI_PSETOP_NULL if setop already applied */
            setop = MPI_PSETOP_CANCEL;
            input_psets = (char **)malloc(1 * sizeof(char *));                                                              
            input_psets[0] = strdup(data->main_pset);                                                                             
            noutput = 0;                                                                                                    
            MPI_Session_dyn_v2a_psetop(data->session, &setop, input_psets, 1, &output_psets, &noutput, MPI_INFO_NULL);       
            free_string_array(&input_psets, 1);
            free_string_array(&output_psets, noutput);
            if (MPI_PSETOP_NULL == setop)                                                                                  
            {
                MPI_Wait(&data->request, MPI_STATUS_IGNORE);
                if (MPI_PSETOP_NULL != data->setop)                                                                            
                {                                                                                                           
                    if (0 != strcmp("", data->output_psets[1]))                                                                 
                    {                                                                                                       
                        
                        /* Inform spawned processes about cancelation */
                        MPI_Info_create(&info);                                                                             
                        MPI_Info_set(info, "dyn_pset", "dyn_psets://canceled");                                                  
                        MPI_Session_set_pset_data(data->session, data->output_psets[1], info);
                        MPI_Info_free(&info);  

                        /* Wait for acknowledgement of spawned processes */
                        MPI_Session_get_pset_data (data->session, "mpi://WORLD", data->output_psets[1], &key, 1, true, &info);
                        MPI_Info_get(info, key, MPI_MAX_PSET_NAME_LEN, ack_response, &flag);
                        if(!flag){
                            printf("ERROR: No ack received from spawned processes\n");
                            dyn_pset_state_free(state);
                            return 1;
                        }

                        MPI_Info_free(&info);
                    }                                                                   
                    MPI_Session_dyn_finalize_psetop(data->session, data->main_pset);            
                    free_string_array(&data->output_psets, data->noutput_psets);                                            
                }                                                                       
            }
        }                                                                           
    }

    //dyn_pset_state_free(state);

    return 0;
}

int dyn_pset_set_info(dyn_pset_state_t * state, MPI_Info info){
    dyn_pset_data_t *data = (dyn_pset_data_t *) state->dyn_pset_data;

    if(data->info != MPI_INFO_NULL){
        MPI_Info_free(&data->info);
    }

    if(info == MPI_INFO_NULL){
        data->info = MPI_INFO_NULL;
    }else{
        MPI_Info_dup(info, &data->info);
    }

    return 0;
}