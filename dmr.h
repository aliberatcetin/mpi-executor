#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"
#include <unistd.h>
#include <string>
#include <vector>
#include <sys/types.h>
#include <signal.h>
#include <iostream>
#include <string.h>

#define STR_SIZE 2048 /**< @brief Standard size for strings. **/

char _keys[][MPI_MAX_INFO_KEY] = {"mpi_dyn", "mpi_primary", "inter_pset", "mpi_included", "dmr://finalize"};
char *keys[5] = {_keys[0], _keys[1], _keys[2], _keys[3], _keys[4]};
MPI_Info info = MPI_INFO_NULL;
char main_pset[MPI_MAX_PSET_NAME_LEN];
char delta_pset[MPI_MAX_PSET_NAME_LEN];
char old_main_pset[MPI_MAX_PSET_NAME_LEN];
char final_pset[MPI_MAX_PSET_NAME_LEN];
char mpi_world_pset[] = "mpi://WORLD";
char boolean_string[6];
int flag;
bool dynamic_proc = false;
bool primary_proc = false;
char **input_psets = NULL, **output_psets = NULL, **q_output_psets = NULL;
char str[MPI_MAX_PSET_NAME_LEN];
MPI_Group wgroup = MPI_GROUP_NULL;
int noutput = 0, noutput2 = 0, op_req = MPI_PSETOP_NULL, op_query = MPI_PSETOP_NULL;

int DMR_it, DMR_STEPS;
MPI_Comm DMR_INTERCOMM = MPI_COMM_NULL;
MPI_Comm DMR_COMM_OLD = MPI_COMM_NULL;
MPI_Comm DMR_COMM_NEW = MPI_COMM_NULL;
int DMR_comm_rank, DMR_comm_size, DMR_comm_prev_size;
MPI_Session DMR_session = MPI_SESSION_NULL;
MPI_Request psetop_req = MPI_REQUEST_NULL;
int DMR_min, DMR_max, DMR_pref;

#define GET_MACRO(_0, _1, NAME, ...) NAME
#define DMR_FINALIZE(...) GET_MACRO(_0, ##__VA_ARGS__, DMR_FINALIZE1, DMR_FINALIZE0)(__VA_ARGS__)

#define DMR_INIT(steps, FUNC_INIT, EXPAND_RECV)                                                                                    \
    {                                                                                                                              \
        MPI_Session_init(MPI_INFO_NULL, MPI_ERRORS_ARE_FATAL, &DMR_session);                                                       \
        strcpy(main_pset, "mpi://WORLD");                                                                                          \
        strcpy(delta_pset, "");                                                                                                    \
        MPI_Session_get_pset_info(DMR_session, main_pset, &info);                                                                  \
        MPI_Info_get(info, "mpi_dyn", 6, boolean_string, &flag);                                                                   \
        if (flag && 0 == strcmp(boolean_string, "True"))                                                                           \
        {                                                                                                                          \
            MPI_Info_get(info, "mpi_primary", 6, boolean_string, &flag);                                                           \
            if (flag && 0 == strcmp(boolean_string, "True"))                                                                       \
            {                                                                                                                      \
                primary_proc = true;                                                                                               \
            }                                                                                                                      \
            MPI_Info_free(&info);                                                                                                  \
            dynamic_proc = true;                                                                                                   \
            MPI_Session_get_pset_data(DMR_session, main_pset, main_pset, (char **)&keys[2], 1, true, &info);                       \
            MPI_Info_get(info, "inter_pset", MPI_MAX_PSET_NAME_LEN, main_pset, &flag);                                             \
            MPI_Info_free(&info);                                                                                                  \
            if (0 == strcmp(main_pset, "dmr://finalize"))                                                                          \
            {                                                                                                                      \
                if (primary_proc)                                                                                                  \
                {                                                                                                                  \
                    MPI_Info_create(&info);                                                                                        \
                    MPI_Info_set(info, "dmr://finalize", "ack");                                                                   \
                    MPI_Session_set_pset_data(DMR_session, "mpi://WORLD", info);                                                   \
                    MPI_Info_free(&info);                                                                                          \
                }                                                                                                                  \
                MPI_Session_finalize(&DMR_session);                                                                                \
                return 0;                                                                                                          \
            }                                                                                                                      \
            MPI_Session_get_pset_info(DMR_session, main_pset, &info);                                                              \
        }                                                                                                                          \
        MPI_Info_get(info, "mpi_primary", 6, boolean_string, &flag);                                                               \
        if (flag && 0 == strcmp(boolean_string, "True"))                                                                           \
        {                                                                                                                          \
            primary_proc = true;                                                                                                   \
        }                                                                                                                          \
        else                                                                                                                       \
        {                                                                                                                          \
            primary_proc = false;                                                                                                  \
        }                                                                                                                          \
        MPI_Info_free(&info);                                                                                                      \
        MPI_Group wgroup = MPI_GROUP_NULL;                                                                                         \
        MPI_Group_from_session_pset(DMR_session, main_pset, &wgroup);                                                              \
        MPI_Comm_create_from_group(wgroup, "mpi.forum.example", MPI_INFO_NULL, MPI_ERRORS_RETURN, &DMR_INTERCOMM);                 \
        DMR_COMM_NEW = DMR_INTERCOMM;                                                                                              \
        MPI_Comm_rank(DMR_INTERCOMM, &DMR_comm_rank);                                                                              \
        MPI_Comm_size(DMR_INTERCOMM, &DMR_comm_size);                                                                              \
        /*printf("Before INIT [%d/%d] %d: %s(%s,%d)\n", DMR_comm_rank, DMR_comm_size, getpid(), __FILE__, __func__, __LINE__);  */\
        if (!dynamic_proc)                                                                                                         \
        {                                                                                                                          \
            DMR_it = 0;                                                                                                            \
            FUNC_INIT;                                                                                                             \
        }                                                                                                                          \
        else                                                                                                                       \
        {                                                                                                                          \
            /*FUNC_INIT;*/                                                                                                             \
            /*printf("Before Bcast [%d/%d] %d: %s(%s,%d)\n", DMR_comm_rank, DMR_comm_size, getpid(), __FILE__, __func__, __LINE__); */ \
            MPI_Bcast(&DMR_it, 1, MPI_INT, 0, DMR_INTERCOMM);                                                                      \
            DMR_it++;                                                                                                              \
            EXPAND_RECV;                                                                                                           \
        }                                                                                                                          \
        MPI_Group_free(&wgroup);                                                                                                   \
        strcpy(final_pset, main_pset);                                                                                             \
        DMR_STEPS = steps;                                                                                                         \
    }

#define DMR_RECONFIGURATION(EXPAND_SEND, EXPAND_RECV, SHRINK_SEND, SHRINK_RECV)                                                                          \
    {                                                                                                                                                    \
        if (DMR_it < DMR_STEPS)                                                                                                                          \
        {                                                                                                                                                \
            noutput = 0;                                                                                                                                 \
            if (primary_proc && psetop_req == MPI_REQUEST_NULL)                                                                                          \
            {                                                                                                                                            \
                MPI_Info_create(&info);                                                                                                                  \
                sprintf(str, "%d", DMR_min);                                                                                                             \
                MPI_Info_set(info, "mpi_num_procs_min", str);                                                                                            \
                sprintf(str, "%d", DMR_max);                                                                                                             \
                MPI_Info_set(info, "mpi_num_procs_max", str);                                                                                            \
                sprintf(str, "%d", DMR_pref);                                                                                                            \
                MPI_Info_set(info, "mpi_num_procs_pref", str);                                                                                           \
                input_psets = (char **)malloc(1 * sizeof(char *));                                                                                       \
                input_psets[0] = strdup(main_pset);                                                                                                      \
                op_req = MPI_PSETOP_REPLACE;                                                                                                             \
                MPI_Session_dyn_v2a_psetop_nb(DMR_session, &op_req, input_psets, 1, &output_psets, &noutput, info, &psetop_req);                         \
                MPI_Info_free(&info);                                                                                                                    \
            }                                                                                                                                            \
            /* Query if there is a resource change */                                                                                                    \
            noutput2 = 0;                                                                                                                                \
            MPI_Session_dyn_v2a_query_psetop(DMR_session, main_pset, main_pset, &op_query, &q_output_psets, &noutput2);                                  \
            if (MPI_PSETOP_NULL != op_query)                                                                                                             \
            {                                                                                                                                            \
                MPI_Comm_dup(DMR_INTERCOMM, &DMR_COMM_OLD);                                                                                              \
                DMR_COMM_NEW = MPI_COMM_NULL;                                                                                                            \
                /* Publish name of inter-pset */                                                                                                         \
                if (primary_proc)                                                                                                                        \
                {                                                                                                                                        \
                    MPI_Request_free(&psetop_req);                                                                                                       \
                    MPI_Info_create(&info);                                                                                                              \
                    MPI_Info_set(info, "inter_pset", output_psets[2]);                                                                                   \
                    MPI_Session_set_pset_data(DMR_session, output_psets[1], info);                                                                       \
                    MPI_Info_free(&info);                                                                                                                \
                    free_string_array(output_psets, noutput);                                                                                            \
                    free_string_array(input_psets, 1);                                                                                                   \
                }                                                                                                                                        \
                strcpy(final_pset, q_output_psets[0]);                                                                                                   \
                MPI_Session_get_pset_data(DMR_session, main_pset, q_output_psets[1], (char **)&keys[2], 1, true, &info);                                 \
                strcpy(old_main_pset, main_pset);                                                                                                        \
                MPI_Info_get(info, "inter_pset", MPI_MAX_PSET_NAME_LEN, main_pset, &flag);                                                               \
                MPI_Info_free(&info);                                                                                                                    \
                /* Check if this process is the primary process of the new pset */                                                                       \
                MPI_Session_get_pset_info(DMR_session, main_pset, &info);                                                                                \
                MPI_Info_get(info, "mpi_primary", 6, boolean_string, &flag);                                                                             \
                primary_proc = (0 == strcmp(boolean_string, "True"));                                                                                    \
                /* Check if this process is included in the new PSet */                                                                                  \
                MPI_Info_get(info, "mpi_included", 6, boolean_string, &flag);                                                                            \
                MPI_Info_free(&info);                                                                                                                    \
                /* Create new communicator */                                                                                                            \
                MPI_Comm_disconnect(&DMR_INTERCOMM);                                                                                                     \
                /*printf("Before New Intercomm [%d/%d] %d: %s(%s,%d)\n", DMR_comm_rank, DMR_comm_size, getpid(), __FILE__, __func__, __LINE__);       */ \
                if (0 != strcmp(boolean_string, "False"))                                                                                                \
                {                                                                                                                                        \
                    strcpy(final_pset, main_pset);                                                                                                       \
                    MPI_Group_from_session_pset(DMR_session, main_pset, &wgroup);                                                                        \
                    MPI_Comm_create_from_group(wgroup, "mpi.forum.example", MPI_INFO_NULL, MPI_ERRORS_RETURN, &DMR_INTERCOMM);                           \
                    MPI_Group_free(&wgroup);                                                                                                             \
                    MPI_Comm_rank(DMR_INTERCOMM, &DMR_comm_rank);                                                                                        \
                    MPI_Comm_size(DMR_INTERCOMM, &DMR_comm_size);                                                                                        \
                    /*printf("Before Bcast [%d/%d] %d: %s(%s,%d)\n", DMR_comm_rank, DMR_comm_size, getpid(), __FILE__, __func__, __LINE__);          */  \
                    MPI_Bcast(&DMR_it, 1, MPI_INT, 0, DMR_INTERCOMM);                                                                                    \
                    DMR_COMM_NEW = DMR_INTERCOMM;                                                                                                        \
                }                                                                                                                                        \
                /* Do data reditribution with DMR_COMM_OLD and DMR_COMM_NEW defined */                                                                   \
                /*printf("Before Data Redistribution [%d/%d] %d: %s(%s,%d)\n", DMR_comm_rank, DMR_comm_size, getpid(), __FILE__, __func__, __LINE__); */ \
                /* Expansion */                                                                                                                          \
                if (0 != strcmp("", q_output_psets[1]))                                                                                                  \
                {                                                                                                                                        \
                    EXPAND_SEND;                                                                                                                         \
                }                                                                                                                                        \
                /* Shrink*/                                                                                                                              \
                if (0 != strcmp("", q_output_psets[0]))                                                                                                  \
                {                                                                                                                                        \
                    /* Staying*/                                                                                                                         \
                    if (0 != strcmp(boolean_string, "False"))                                                                                            \
                    {                                                                                                                                    \
                        SHRINK_RECV;                                                                                                                     \
                    }                                                                                                                                    \
                    else                                                                                                                                 \
                    {                                                                                                                                    \
                        SHRINK_SEND;                                                                                                                     \
                    }                                                                                                                                    \
                }                                                                                                                                        \
                free_string_array(q_output_psets, noutput2);                                                                                             \
                /* Finalize PSetOp*/                                                                                                                     \
                if (primary_proc)                                                                                                                        \
                {                                                                                                                                        \
                    MPI_Session_dyn_finalize_psetop(DMR_session, old_main_pset);                                                                         \
                }                                                                                                                                        \
                MPI_Comm_disconnect(&DMR_COMM_OLD);                                                                                                      \
                if (0 == strcmp(boolean_string, "False"))                                                                                                \
                {                                                                                                                                        \
                    /* Leaving Processes */                                                                                                              \
                    break;                                                                                                                               \
                }                                                                                                                                        \
            }                                                                                                                                            \
        }                                                                                                                                                \
    }

#define DMR_FINALIZE1(FUNC_FINALIZE)                                                                                           \
    {                                                                                                                          \
        FUNC_FINALIZE;                                                                                                         \
        /* We need to cancel our last pset operation. If it was already executed we need to tell any new procs to terminate */ \
        if (primary_proc)                                                                                                      \
        {                                                                                                                      \
            op_req = MPI_PSETOP_CANCEL;                                                                                        \
            input_psets = (char **)malloc(1 * sizeof(char *));                                                                 \
            input_psets[0] = strdup(main_pset);                                                                                \
            noutput = 0;                                                                                                       \
            MPI_Session_dyn_v2a_psetop(DMR_session, &op_req, input_psets, 1, &output_psets, &noutput, MPI_INFO_NULL);          \
            free_string_array(input_psets, 1);                                                                                 \
            if (MPI_PSETOP_NULL == op_req)                                                                                     \
            {                                                                                                                  \
                strcpy(delta_pset, "mpi://SELF");                                                                              \
                MPI_Session_dyn_v2a_query_psetop(DMR_session, delta_pset, main_pset, &op_query, &q_output_psets, &noutput);    \
                if (MPI_PSETOP_NULL != op_query)                                                                               \
                {                                                                                                              \
                    if (0 != strcmp("", q_output_psets[1]))                                                                    \
                    {                                                                                                          \
                        MPI_Info_create(&info);                                                                                \
                        MPI_Info_set(info, "inter_pset", "dmr://finalize");                                                    \
                        MPI_Session_set_pset_data(DMR_session, q_output_psets[1], info);                                       \
                        MPI_Info_free(&info);                                                                                  \
                    }                                                                                                          \
                    MPI_Session_dyn_finalize_psetop(DMR_session, main_pset);                                                   \
                    MPI_Session_get_pset_data(DMR_session, delta_pset, q_output_psets[1], (char **)&keys[4], 1, true, &info);  \
                    free_string_array(q_output_psets, noutput);                                                                \
                    MPI_Info_free(&info);                                                                                      \
                }                                                                                                              \
            }                                                                                                                  \
        }                                                                                                                      \
        input_psets = (char **)malloc(1 * sizeof(char *));                                                                     \
        input_psets[0] = strdup(final_pset);                                                                                   \
        MPI_Session_pset_barrier(DMR_session, input_psets, 1, NULL);                                                           \
        free_string_array(input_psets, 1);                                                                                     \
        if (MPI_COMM_NULL != DMR_INTERCOMM)                                                                                    \
        {                                                                                                                      \
            MPI_Comm_disconnect(&DMR_INTERCOMM);                                                                               \
        }                                                                                                                      \
        MPI_Session_finalize(&DMR_session);                                                                                    \
    }

#define DMR_FINALIZE0()                                                                                                        \
    {                                                                                                                          \
        /* We need to cancel our last pset operation. If it was already executed we need to tell any new procs to terminate */ \
        if (primary_proc)                                                                                                      \
        {                                                                                                                      \
            op_req = MPI_PSETOP_CANCEL;                                                                                        \
            input_psets = (char **)malloc(1 * sizeof(char *));                                                                 \
            input_psets[0] = strdup(main_pset);                                                                                \
            noutput = 0;                                                                                                       \
            MPI_Session_dyn_v2a_psetop(DMR_session, &op_req, input_psets, 1, &output_psets, &noutput, MPI_INFO_NULL);          \
            free_string_array(input_psets, 1);                                                                                 \
            if (MPI_PSETOP_NULL == op_req)                                                                                     \
            {                                                                                                                  \
                strcpy(delta_pset, "mpi://SELF");                                                                              \
                MPI_Session_dyn_v2a_query_psetop(DMR_session, delta_pset, main_pset, &op_query, &q_output_psets, &noutput);    \
                if (MPI_PSETOP_NULL != op_query)                                                                               \
                {                                                                                                              \
                    if (0 != strcmp("", q_output_psets[1]))                                                                    \
                    {                                                                                                          \
                        MPI_Info_create(&info);                                                                                \
                        MPI_Info_set(info, "inter_pset", "dmr://finalize");                                                    \
                        MPI_Session_set_pset_data(DMR_session, q_output_psets[1], info);                                       \
                        MPI_Info_free(&info);                                                                                  \
                    }                                                                                                          \
                    MPI_Session_dyn_finalize_psetop(DMR_session, main_pset);                                                   \
                    MPI_Session_get_pset_data(DMR_session, delta_pset, q_output_psets[1], (char **)&keys[4], 1, true, &info);  \
                    free_string_array(q_output_psets, noutput);                                                                \
                    MPI_Info_free(&info);                                                                                      \
                }                                                                                                              \
            }                                                                                                                  \
        }                                                                                                                      \
        input_psets = (char **)malloc(1 * sizeof(char *));                                                                     \
        input_psets[0] = strdup(final_pset);                                                                                   \
        MPI_Session_pset_barrier(DMR_session, input_psets, 1, NULL);                                                           \
        free_string_array(input_psets, 1);                                                                                     \
        if (MPI_COMM_NULL != DMR_INTERCOMM)                                                                                    \
        {                                                                                                                      \
            MPI_Comm_disconnect(&DMR_INTERCOMM);                                                                               \
        }                                                                                                                      \
        MPI_Session_finalize(&DMR_session);                                                                                    \
    }

void free_string_array(char **array, int size)
{
    int i;
    if (0 == size)
    {
        return;
    }
    for (i = 0; i < size; i++)
    {
        free(array[i]);
    }
    free(array);
}

int DMR_Reconfiguration(char *argv[], MPI_Comm *DMR_INTERCOMM, int min, int max, int step, int pref);
void DMR_Send_expand(double *data, int size, MPI_Comm DMR_INTERCOM);
void DMR_Recv_expand(double **data, int *size, MPI_Comm DMR_INTERCOM);
void DMR_Send_shrink(double *data, int size, MPI_Comm DMR_INTERCOM);
void DMR_Recv_shrink(double **data, int *size, MPI_Comm DMR_INTERCOM);
void DMR_Set_parameters(int min, int max, int pref);
