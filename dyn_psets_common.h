#ifndef DYN_PSETS_COMMON_H
#define DYN_PSETS_COMMON_H

#include "mpi.h"
#include "stdbool.h"

typedef struct{
    void * dyn_pset_data;
    void * user_pointer;
    bool is_dynamic;
    MPI_Comm mpicomm;
    int mpirank;
    int mpisize;
    bool is_master;
    bool user_task_flag;
} dyn_pset_state_t;

typedef struct
{
    bool is_master;
    bool is_dynamic;
    int cur_iter;
    int step_size;
    int try;
}reconf_info_t;

typedef int (*dyn_psets_expand_send_func_t) (dyn_pset_state_t * state);
typedef int (*dyn_psets_expand_recv_func_t) (dyn_pset_state_t * state);
typedef int (*dyn_psets_shrink_send_func_t) (dyn_pset_state_t * state);
typedef int (*dyn_psets_shrink_recv_func_t) (dyn_pset_state_t * state);
#endif /* !DYN_PSETS_COMMON_H */
