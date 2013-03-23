/*
 * This file contains a special functions, structs and defines
 * for dataflow network interaction.
 */

#ifndef DATAFLOW_NETWORK_H
#define DATAFLOW_NETWORK_H

#include "dataflow_utilities.h"

/* Dataflow network define. */
#define DATAFLOW_TYPE_MAIN_AGENT           "http://yar.fruct.org/projects/fcpk/wiki/dataflow-agent"
#define DATAFLOW_TYPE_SUBSTITUTE_AGENT     "http://yar.fruct.org/projects/fcpk/wiki/substitute-agent"
#define DATAFLOW_MAIN_AGENT_PREFIX         "http://yar.fruct.org/projects/fcpk/wiki/dataflow-agent#"
#define DATAFLOW_SUBSTITUTE_AGENT_PREFIX   "http://yar.fruct.org/projects/fcpk/wiki/substitute-agent#"
#define DATAFLOW_TYPE                      "Type"
#define DATAFLOW_NONE                      "None"
#define DATAFLOW_YES                       "yes"
#define DATAFLOW_NO                        "no"
#define DATAFLOW_CHANNEL                   "DataflowChannel"
#define DATAFLOW_ACTIVE                    "Active"
#define DATAFLOW_SUBSTITUTES               "Substitutes"
#define DATAFLOW_SUBSTITUTE_PROGRAM        "SubstituteProgram"
#define DATAFLOW_SUBSTITUTE_PROGRAM_TYPE   "SubstituteProgramType"
#define DATAFLOW_DESCRIPTION               "Description"

/*
 * This structure contains a data for a inner storage entry.
 * A kp_id field needs for indication of a agent which associate with
 * a inner storage entry.
 */
typedef struct {
    GSList* triple_list;
    gchar* kp_id;
} dataflow_inner_storage_entry;

/*
 * This structure contains a triple data for a triple list entry in a inner storage entry.
 * If a value field is equal true then triple was added. Else triple was removed.
 */
typedef struct {
    ssTriple_t* triple;
    gboolean is_added;
} dataflow_inner_storage_triple;

/* Substitute program information. */
typedef struct {
    GSList* subscriptions;
    GSList* results;
    GSList* states;
} dataflow_subst_program;

/* This structure contains a data for a main agent entry. */
typedef struct {
    gchar* kp_id;
    gchar* agent_id;
    gchar* substitute_program_type;
    dataflow_subst_program* substitute_program;

    gchar* substitute_agent_kp_id;

    gboolean is_work;
    GCond* work_cond;
    /* A temp kp_id field that must be assigned to the main agent after returning. */
    gchar* temp_kp_id;
    dataflow_network_conn_data* network_data;

    /* Indicate about a status of receiving a connection information. */
    GCond* receive_conn_cond;
    gboolean is_receive_conn;
} dataflow_main_agent_entry;

typedef enum {
    REST = 0,
    SUBSTITUTE = 1,
    WAIT_START_CONFIRMATION = 2,
    WAIT_FINISH_CONFIRMATION = 3
} dataflow_subst_agent_state;

/* This structure contains a data for a substitute agent entry. */
typedef struct {
    gchar* agent_id;
    gchar* substitute_program_type;
    gchar* kp_id;

    gchar* substitutes_kp_id;

    dataflow_subst_agent_state state;
    GCond* substitute_cond;
    dataflow_network_conn_data* network_data;
} dataflow_subst_agent_entry;

void dataflow_try_to_add_agent(sib_data_structure* sib, GSList* added_triples, char* kp_id);

void dataflow_run_network_operation(sib_data_structure* sib);

void dataflow_try_to_substitute_broken_agent(sib_data_structure* sib, gchar* kp_id);

void dataflow_initialize_variables(sib_data_structure* sib);

void dataflow_save_connection_info_about_agent(sib_data_structure* sib,
        ssap_message_header* header,
        GSList* request_triples,
        ssap_sib_message* response,
        DBusConnection* connection);

#endif  /* DATAFLOW_NETWORK_H */
