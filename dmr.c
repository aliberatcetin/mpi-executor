#include "dmr.h"

void DMR_Send_shrink(double *data, int size, MPI_Comm DMR_INTERCOMM)
{
    // printf("(sergio): %s(%s,%d)\n", __FILE__, __func__, __LINE__);
    int my_rank, comm_size, intercomm_size, factor, dst;
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
    MPI_Comm_remote_size(DMR_INTERCOMM, &intercomm_size);
    factor = comm_size / intercomm_size;
    dst = my_rank / factor;
    MPI_Send(data, size, MPI_DOUBLE, dst, 0, DMR_INTERCOMM);
}

void DMR_Recv_shrink(double **data, int *size, MPI_Comm DMR_INTERCOMM)
{
    // printf("(sergio): %s(%s,%d)\n", __FILE__, __func__, __LINE__);
    int my_rank, comm_size, parent_size, src, factor, iniPart, i, parent_data_size;
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
    MPI_Comm_remote_size(DMR_INTERCOMM, &parent_size);
    MPI_Status status;
    factor = parent_size / comm_size;
    *data = (double *)malloc((*size) * sizeof(double));
    for (i = 0; i < factor; i++)
    {
        src = my_rank * factor + i;
        iniPart = parent_data_size * i;
        MPI_Recv((*data) + iniPart, (*size), MPI_DOUBLE, src, 0, DMR_INTERCOMM, &status);
    }
}

void DMR_Send_expand(double *data, int size, MPI_Comm DMR_INTERCOMM)
{
    // printf("(sergio): %s(%s,%d): %d\n", __FILE__, __func__, __LINE__, size);
    int my_rank, comm_size, intercomm_size, localSize, factor, dst;
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
    MPI_Comm_remote_size(DMR_INTERCOMM, &intercomm_size);
    factor = intercomm_size / comm_size;
    localSize = size / factor;
    for (int i = 0; i < factor; i++)
    {
        dst = my_rank * factor + i;
        int iniPart = localSize * i;
        // printf("(sergio): %s(%s,%d): %d send to %d (%d)\n", __FILE__, __func__, __LINE__, my_rank, dst, localSize);
        MPI_Send(data + iniPart, localSize, MPI_DOUBLE, dst, 0, DMR_INTERCOMM);
    }
}

void DMR_Recv_expand(double **data, int *size, MPI_Comm DMR_INTERCOMM)
{
    // printf("(sergio): %s(%s,%d): %d\n", __FILE__, __func__, __LINE__, *size);
    int my_rank, comm_size, parent_size, src, factor;
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
    MPI_Comm_remote_size(DMR_INTERCOMM, &parent_size);
    MPI_Status status;
    factor = comm_size / parent_size;
    *data = (double *)malloc((*size) * sizeof(double));
    src = my_rank / factor;
    // printf("(sergio): %s(%s,%d): %d recv from %d (%d)\n", __FILE__, __func__, __LINE__, my_rank, src, *size);
    MPI_Recv(*data, *size, MPI_DOUBLE, src, 0, DMR_INTERCOMM, &status);
}


void DMR_Set_parameters(int min, int max, int pref){
    DMR_min = min;
    DMR_max = max;
    DMR_pref = pref;
}


/*
int DMR_Reconfiguration(char *argv[], MPI_Comm *DMR_INTERCOMM, int min, int max, int step, int pref)
{
    int world_size, world_rank, name_len;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    char processor_name[MPI_MAX_PROCESSOR_NAME];
    MPI_Get_processor_name(processor_name, &name_len);
    int action = 0, nc = 0, counter = 0;
    std::vector<int> hostInstances;
    std::vector<std::string> tokensParams;
    std::vector<std::string> tokensHost;
    int dependentJobId;

    if (world_rank == 0)
    {
        char *pID = getenv("SLURM_JOBID");
        uint32_t procID = atoi(pID);
        dependentJobId = procID;  
        job_info_msg_t *MasterInfo;
        slurm_load_job(&MasterInfo, procID, 0);
        // Waiting for stabilisation version without sincronous sockets
        while (world_size != (int)MasterInfo->job_array->num_nodes)
        {
            printf("waiting for stabilization... (current %d - slurm %d)\n", world_size, MasterInfo->job_array->num_nodes);
            sleep(5);
            slurm_free_job_info_msg(MasterInfo);
            slurm_load_job(&MasterInfo, procID, 0);
        }
        // Slurm chooses the action in the selection policy
        _dmr_slurm_jobinfo_t *jobinfo = (_dmr_slurm_jobinfo_t *)malloc(sizeof(_dmr_slurm_jobinfo_t));
        MasterInfo->job_array->select_jobinfo->data = jobinfo;
        jobinfo->job_id = (uint32_t)procID;
        jobinfo->min = (uint32_t)min;
        jobinfo->max = (uint32_t)max;
        jobinfo->preference = (uint32_t)pref;
        jobinfo->step = (uint32_t)step;
        jobinfo->currentNodes = (uint32_t)world_size;
        slurm_get_select_jobinfo(MasterInfo->job_array->select_jobinfo, SELECT_JOBDATA_ACTION, NULL);

        action = jobinfo->action;
        nc = jobinfo->resultantNodes;

        printf("[%d]-> %s(%s,%d) || Action: %d %d - Current nodes %d, while in slurm %d\n", getpid(), __func__, __FILE__, __LINE__, jobinfo->action, jobinfo->resultantNodes, world_size, MasterInfo->job_array->num_nodes);
        free(jobinfo);
        slurm_free_job_info_msg(MasterInfo);

        switch (action)
        {
        case 0:
        {
            break;
        }
        case 1:
        { // expand
            job_desc_msg_t job_desc;
            job_info_msg_t *jobAux;
            resource_allocation_response_msg_t *slurm_alloc_msg_ptr;
            slurm_init_job_desc_msg(&job_desc);
            job_desc.name = (char *)"resizer";
            job_desc.dependency = (char *)malloc(sizeof(char) * 64);
            sprintf(job_desc.dependency, (char *)"expand:%d", dependentJobId);
            job_desc.user_id = getuid();
            job_desc.priority = 1000000;
            job_desc.min_nodes = nc - world_size;

            if (slurm_allocate_resources(&job_desc, &slurm_alloc_msg_ptr))
            {
                slurm_perror((char *)"slurm_allocate_resources error (DMRCheckStatus)");
                exit(1);
            }
            free(job_desc.dependency);
            dependentJobId = slurm_alloc_msg_ptr->job_id;
            if (slurm_alloc_msg_ptr->node_list == NULL)
            {
                slurm_load_job(&jobAux, dependentJobId, 0);
                // printf("(sergio): %s(%s,%d) - %d %s\n", __FILE__, __func__, __LINE__, dependentJobId, jobAux->job_array->nodes);
                while (jobAux->job_array->nodes == NULL)
                {
                    slurm_free_job_info_msg(jobAux);
                    printf("Waiting for backfilling (check sinfo or priorities)\n");
                    counter++;
                    sleep(5);
                    if (counter == 10)
                    {
                        printf("Backfilling not completed\n");
                        action = 0;
                        slurm_kill_job(dependentJobId, SIGKILL, 0);
                        slurm_free_resource_allocation_response_msg(slurm_alloc_msg_ptr);
                        dependentJobId = -1;
                        break;
                    }
                    slurm_load_job(&jobAux, dependentJobId, 0);
                }
                if (action != 0)
                    slurm_free_job_info_msg(jobAux);
            }
            break;
        }
        case 2:
        { // shrink
            return 0;
        }
        }
    }
    MPI_Bcast(&action, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&nc, 1, MPI_INT, 0, MPI_COMM_WORLD);
    int *errcodes = (int *) malloc(sizeof(int) * nc);
    switch (action)
    {
    case 0:
    {
        break;
    }
    case 1:
    { // expand
        job_desc_msg_t job_update;
        job_info_msg_t *MasterInfo, *ExpandInfo;
        char *jID = getenv("SLURM_JOBID");
        uint32_t jobID = atoi(jID);
        char *finalNodelist = (char *)malloc(sizeof(char) * STR_SIZE);

        if (world_rank == 0)
        {
            // printf("@@@[%d]-> %s(%s,%d)\n", getpid(), __func__, __FILE__, __LINE__);
            slurm_load_job(&MasterInfo, jobID, 0);
            if (MasterInfo == NULL || MasterInfo->record_count == 0)
            {
                slurm_perror((char *)"%%%%%% No info %%%%%%");
                exit(1);
            }
            slurm_load_job(&ExpandInfo, dependentJobId, 0);
            if (ExpandInfo == NULL || ExpandInfo->record_count == 0)
            {
                slurm_perror((char *)"%%%%%% No info %%%%%%");
                exit(1);
            }

            char currHostnames[2048], appendHostnames[2048];
            readableHostlist(MasterInfo->job_array[0].nodes, currHostnames);
            readableHostlist(ExpandInfo->job_array[0].nodes, appendHostnames);
            sprintf(finalNodelist, "%s,%s", currHostnames, appendHostnames);
            slurm_free_job_info_msg(ExpandInfo);
            slurm_free_job_info_msg(MasterInfo);

            //$ scontrol update jobid=$SLURM_JOBID NumNodes=0
            slurm_init_job_desc_msg(&job_update);
            job_update.job_id = dependentJobId;
            job_update.min_nodes = 0;
            if (slurm_update_job(&job_update))
            {
                slurm_perror((char *)"slurm_update_job error");
                exit(1);
            }
            // exit
            if (slurm_kill_job(dependentJobId, SIGKILL, 0))
            {
                slurm_perror((char *)"slurm_kill_job error");
                exit(1);
            }
            //$ scontrol update jobid=$SLURM_JOBID NumNodes=ALL
            // printf("[%d]-> %s(%s,%d) || %d\n", getpid(), __func__, __FILE__, __LINE__, job.min_nodes);
            slurm_init_job_desc_msg(&job_update);
            job_update.job_id = jobID;
            job_update.min_nodes = INFINITE;
            if (slurm_update_job(&job_update))
            {
                slurm_perror((char *)"slurm_update_job error");
                exit(1);
            }
            //printf("(sergio): %s(%s,%d) %s\n", __FILE__, __func__, __LINE__, finalNodelist);
        }
        
        MPI_Bcast(finalNodelist, STR_SIZE + 1, MPI_CHAR, 0, MPI_COMM_WORLD);

        hostlist_t hl = slurm_hostlist_create(finalNodelist);
        SLURMbuildHostLists(hl, tokensParams, tokensHost, hostInstances);
        slurm_hostlist_destroy(hl);
        
        std::vector<char *> commands(nc, argv[0]);
        std::vector<int> num_processes(nc, 1);
        std::vector<char **> argvs(nc);
        MPI_Info *host_info = (MPI_Info*) malloc(sizeof(MPI_Info) * nc);
        std::string host;

        for (int i = 0; i < nc; i++){
            MPI_Info_create(&host_info[i]);
            host = tokensHost.at(i);
            MPI_Info_set(host_info[i], "host", host.c_str());
        }

		//instead of slurm, we can query the runtime for getting the operation

		//query pset_op:
		//It returns de operations (that we are going to ignore because we already know it)
		// and the new pset.
			
		//MPI_Sessions_set_pset_data (an A process publishes the inter_pset_name in the dictionary of B pset)

		//MPI_Session_group_from_pset (interpset)
		//MPI_Comm_from_group (intergroup)

        //printf("(sergio): %s(%s,%d) nc %d %s\n", __FILE__, __func__, __LINE__, nc, finalNodelist);
        MPI_Comm_spawn_multiple(nc, &commands.front(), &argvs.front(), &num_processes.front(), host_info, 0, MPI_COMM_WORLD, DMR_INTERCOMM, errcodes);
        //MPI_Comm_spawn(argv[0], &argv[1], nc, MPI_INFO_NULL, 0, MPI_COMM_WORLD, DMR_INTERCOMM, errcodes);
        // printf("(sergio): %s(%s,%d)\n", __FILE__, __func__, __LINE__);
        break;
    }
    case 2:
    { // shrink
        MPI_Comm_spawn(argv[0], &argv[1], nc, MPI_INFO_NULL, 0, MPI_COMM_WORLD, DMR_INTERCOMM, errcodes);
        // nanosShrinkJob(MPI_COMM_WORLD, *nnodes, 1, intercomm, true, NULL, 0, NULL);
        break;
    }
    }
    // printf("[%d/%d](sergio): %s(%s,%d) %lu %d\n", world_rank, world_size, __FILE__, __func__, __LINE__, (unsigned long int) *socketThread, *nnodes);
    return action;
}
*/