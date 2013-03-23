/*
 * This file contains a helpful functions, structs and defines
 * for dataflow network interaction but not contained in
 * the dataflow network model description.
 */

#ifndef DATAFLOW_UTILITIES_H
#define DATAFLOW_UTILITIES_H

#include "sib_operations.h"

/* The transaction id for a operation that calls by the SIB. */
#define DATAFLOW_INNER_OPERATION_TRANSACTION_ID 2

/* Protection mechanism constants. */
#define DATAFLOW_PROTECTION_ID_PREFIX "http://yar.fruct.org/projects/fcpk/wiki/dataflow_random_id#"

/* Dataflow function type that is a pointer to a function. */
typedef void (*DataflowFunc)(gpointer);

/* This structure contains a network connection data of an agent. */
typedef struct {
    DBusConnection* conn;
    gchar* space_id;
    gint tr_id;
    ssap_sib_message* rsp;
} dataflow_network_conn_data;

/*
 * This structure contains a data for interaction between
 * the inner subscription and a called function.
 */
typedef struct {
    sib_data_structure* sib;
    GSList* inner_subscr_triples;
    DataflowFunc handler;
    gchar* kp_id;
    /* Triples that was returned from inner subscription. */
    GSList* added_triples;
    GSList* removed_triples;
} dataflow_inner_subscr;

/*
 * This structure contains a data for interaction between
 * the inner subscription and a scheduler.
 */
typedef struct {
    sub_status status;
    gchar* subscr_id;
    gchar* kp_id;

    /* Need to sync with subscribe and unsubscribe. */
    GCond* unsubscr_cond;
    gboolean unsubscr;

    scheduler_item* scheduler_item;
    gpointer subscr_function;
} dataflow_subscr_state;

void dataflow_create_inner_subscription(sib_data_structure* sib, GSList* insert_triples,
                                        DataflowFunc function, gchar* kp_id);

void dataflow_send_subscription_indication_message(GSList* insert_triples,
        GSList* removed_triples, gchar* kp_id, dataflow_network_conn_data* network_data);

void dataflow_set_inner_subscr_to_pending(gpointer subscr_id, gpointer subscr_data,
        gpointer unused);

GSList* dataflow_copy_triple_list(GSList* triple_list);

ssTriple_t* dataflow_create_triple(char* subject, char* predicate,
                                   char* object, ssElementType_t obj_type);

GSList* dataflow_inner_query(sib_data_structure* sib, GSList* triple_list);

void dataflow_inner_update(sib_data_structure* sib, GSList* insert_triples,
                           GSList* removed_triples, char* kp_id);

ssTriple_t* dataflow_find_triple_by_predicate_and_object(GSList* input_list,
        char* predicate, char* object);

ssTriple_t* dataflow_find_triple_by_predicate(GSList* input_list, char* predicate);

GSList* dataflow_append_triple_to_list(GSList* input_list, char* subject, char* predicate,
                                       char* object, ssElementType_t object_type);

void dataflow_setup_triple_protection(sib_data_structure* sib, char* subject,
                                      char* kp_id, GSList* property_list);

void dataflow_stop_inner_subscription(sib_data_structure* sib, gchar* kp_id,
                                      DataflowFunc function, boolean is_need_to_wait);

void dataflow_remove_triple_protection(sib_data_structure* sib, char* subject,
                                       char* kp_id, GSList* property_list);

#endif  /* DATAFLOW_UTILITIES_H */
