#include "mpi.h"
#include "dyn_psets_internal.h"
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>


dyn_pset_data_t * dyn_pset_data_new(const char *pset,reconf_info_t *info_, dyn_psets_expand_send_func_t exp_send, dyn_psets_expand_recv_func_t exp_recv, dyn_psets_shrink_send_func_t shr_send, dyn_psets_shrink_recv_func_t shr_recv){
    int flag = 0;
    char boolean_string[10];
    char *key = "dyn_pset";
    MPI_Info info;
    

    dyn_pset_data_t *data = (dyn_pset_data_t *) malloc(sizeof(dyn_pset_data_t));
    data->session = MPI_SESSION_NULL;
    data->info = MPI_INFO_NULL;
    data->request = MPI_REQUEST_NULL;
    data->setop_pending = false;
    data -> is_master = info_->is_master;
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
    MPI_Session_get_pset_info (data->session, data->main_pset, &info);

    MPI_Info_get(info, "mpi_primary", 6, boolean_string, &flag);
    primary = 0;
    if(0 != strcmp(boolean_string, "True")){
        primary = 1;
    } 
    /* get value for the 'mpi_dyn' key -> if true, this process was added dynamically */
    MPI_Info_get(info, "mpi_dyn", 6, boolean_string, &flag);

    /* if mpi://WORLD is a dynamic PSet retrieve the name of the main PSet stored on mpi://WORLD */
    if(!data->is_master && flag && 0 == strcmp(boolean_string, "True")){
        MPI_Info_free(&info);
        
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

dyn_pset_state_t * dyn_pset_state_new(const char *pset, void * user_pointer,
                            dyn_psets_expand_send_func_t exp_send, dyn_psets_expand_recv_func_t exp_recv,
                            dyn_psets_shrink_send_func_t shr_send, dyn_psets_shrink_recv_func_t shr_recv){
    dyn_pset_state_t * state = malloc(sizeof(dyn_pset_state_t));
    state->mpirank = 0;
    state->mpisize = 0;
    state->is_dynamic = 0;
    reconf_info_t *t = (reconf_info_t*)user_pointer;

    state->is_master = t->is_master;
    state->is_dynamic = t->is_dynamic;
    state->mpicomm = MPI_COMM_NULL;
    state->dyn_pset_data = (void *) dyn_pset_data_new(pset,t, exp_send, exp_recv, shr_send, shr_recv);

    if(NULL == state->dyn_pset_data){
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
