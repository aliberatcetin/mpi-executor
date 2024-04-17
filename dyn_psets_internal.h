#ifndef DYN_PSETS_INTERNAL_H
#define DYN_PSETS_INTERNAL_H

#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>

#include "mpi.h"
#include "dyn_psets_common.h"



typedef struct dyn_pset_data{
  char delta_add[MPI_MAX_PSET_NAME_LEN];
  char delta_sub[MPI_MAX_PSET_NAME_LEN];
  char main_pset[MPI_MAX_PSET_NAME_LEN];
  MPI_Session session;
  MPI_Info info;
  MPI_Request request;
  char **output_psets;
  int noutput_psets;
  int setop;
  int rank;
  bool setop_pending;
  char *task_id;
  bool is_dynamic;
  bool is_master;
  dyn_psets_expand_send_func_t exp_send;
  dyn_psets_expand_recv_func_t exp_recv;
  dyn_psets_shrink_send_func_t shr_send; 
  dyn_psets_shrink_recv_func_t shr_recv;
}dyn_pset_data_t;



#endif /* !DYN_PSETS_INTERNAL_H */
