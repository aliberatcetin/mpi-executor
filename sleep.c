#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <math.h>
#include <string.h>

#include "dmr.h"

#define DURATION 3600
#define STEPS 10
//#define SIZE 1024
#define SIZE 131072
//#define SIZE 268435456 // 2^28
// #define SIZE 4294967296 //2^32 ->  MPI_Send(174)................: MPI_Send(buf=0x2af34c9e3860, count=1, dtype=USER<struct>, dest=12, tag=1200, comm=0x84000000) failed

#define BLOCKSIZE 128
#define NBLOCKS SIZE / BLOCKSIZE

void gather(MPI_Comm comm, double *send_data, double **recv_data){
    int send_size, comm_size, rank;
    //printf("(sergio)[%d/%d] %d: %s(%s,%d)\n", DMR_comm_rank, DMR_comm_size, getpid(), __FILE__, __func__, __LINE__);
    MPI_Comm_size(comm, &comm_size);
    MPI_Comm_rank(comm, &rank);
    send_size = SIZE / comm_size;
    if(rank == 0){
        (*recv_data) = (double *)malloc(SIZE * sizeof(double));
    }
    MPI_Gather(send_data, send_size, MPI_DOUBLE, (*recv_data), send_size, MPI_DOUBLE, 0, comm);
    MPI_Barrier(comm);
}

void scatter(MPI_Comm comm, double **send_data, double**recv_data){
    int recv_size, comm_size, rank;
    //printf("(sergio)[%d/%d] %d: %s(%s,%d)\n", DMR_comm_rank, DMR_comm_size, getpid(), __FILE__, __func__, __LINE__);
    MPI_Comm_size(comm, &comm_size);
    MPI_Comm_rank(comm, &rank);
    recv_size = SIZE / comm_size;
    (*recv_data) = (double *)malloc(recv_size* sizeof(double));
    MPI_Scatter((*send_data), recv_size, MPI_DOUBLE, (*recv_data), recv_size, MPI_DOUBLE, 0, comm);
    if(DMR_comm_rank == 0){
        free(*send_data);
    }
    //printf("(sergio)[%d/%d] %d: %s(%s,%d)\n", DMR_comm_rank, DMR_comm_size, getpid(), __FILE__, __func__, __LINE__);
    //MPI_Barrier(DMR_INTERCOMM);
}

/* Executed only by existent ranks */
void send_expand(double **vector){
    double *gathered_data;
    /* Step 1: Gather on old_comm (to rank 0) */
    gather(DMR_COMM_OLD, *vector, &gathered_data);

    /* Step 2: Scatter on new comm (from rank 0) */
    scatter(DMR_COMM_NEW, &gathered_data, vector);
}

/* Executed only by newly added ranks */
void recv_expand(double **vector){
    double *dummy_ptr;
    /* Step 1: Scatter on new comm (from rank 0) */
    scatter(DMR_COMM_NEW, &dummy_ptr, vector);
}

/* Executed only by leaving processes */
void send_shrink(double **vector){
    double *dummy_ptr;
    /* Step 1: Gather on old_comm (to rank 0) */
    gather(DMR_COMM_OLD, *vector, &dummy_ptr);
}

/* Executed only by staying processes */
void recv_shrink(double **vector){

    double *gathered_data;
    /* Step 1: Gather on old_comm (to rank 0) */
    gather(DMR_COMM_OLD, *vector, &gathered_data);

    /* Step 2: Scatter on new comm (from rank 0) */
    scatter(DMR_COMM_NEW, &gathered_data, vector);
}

void initialize_data(double **vector, int size)
{
    int localsize = size / DMR_comm_size;
    (*vector) = (double *)malloc(localsize * sizeof(double));
    for (int i = 0; i < localsize; i++)
    {
        (*vector)[i] = i * 2;
    }
}

void compute(int i, int bytes, int DMR_comm_rank, int DMR_comm_size, int jobid)
{
    //int durStep = DURATION / DMR_comm_size;
    int durStep = 1;
    if (DMR_comm_rank == 0)
        printf("[%d/%d] COMPUTE Job Id %d: %s(%s,%d) - Step %d doing a sleep of %d seconds (bytes per rank %d)\n", DMR_comm_rank, DMR_comm_size, jobid, __FILE__, __func__, __LINE__, i, durStep, bytes);
    sleep(durStep);
    //MPI_Barrier(DMR_INTERCOMM);
}

// USAGE ./sleepOf <jobid>
int main(int argc, char **argv)
{   
    
    int jobid = atoi(argv[1]);
    double *vector;
    DMR_INIT(STEPS, initialize_data(&vector, SIZE), recv_expand(&vector));
	printf("girdik %d\n",DMR_comm_rank);
    DMR_Set_parameters(8, 64, 32);
    for (;DMR_it < STEPS + 1; DMR_it++){
        printf("size aga: %d\n",DMR_comm_size);
		int localSize = SIZE / DMR_comm_size;
        printf("before computed\n");
		//compute(DMR_it, localSize, DMR_comm_rank, DMR_comm_size, jobid);
        printf("computed\n");
        DMR_RECONFIGURATION(send_expand(&vector), recv_expand(&vector), send_shrink(&vector), recv_shrink(&vector));
        
        printf("(sergio)[%d/%d] %d: %s(%s,%d) Iteration %d\n", DMR_comm_rank, DMR_comm_size, getpid(), __FILE__, __func__, __LINE__, DMR_it);
	}
    
    //printf("(sergio)[%d/%d] %d: %s(%s,%d)\n", DMR_comm_rank, DMR_comm_size, getpid(), __FILE__, __func__, __LINE__);
	DMR_FINALIZE();

	return 0;
}
