#ifndef DYN_PSETS_H
#define DYN_PSETS_H

#include "mpi.h"
#include "dyn_psets_common.h"
#include <stdbool.h>


           
int dyn_pset_adapt(dyn_pset_state_t * state, int *terminate, int *reconfigured);
int dyn_pset_adapt_nb(dyn_pset_state_t * state, int *terminate, int *reconfigured);

int dyn_pset_finalize(dyn_pset_state_t ** state, char *final_pset);

int dyn_pset_set_info(dyn_pset_state_t * state, MPI_Info info);

#endif /* !DYN_PSETS_H */
