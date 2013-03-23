#include "dataflow_utilities.h"

#include <whiteboard_log.h>
#include <whiteboard_util.h>
#include <sib_dbus_ifaces.h>
#include <sibmsg.h>
#include "dataflow_network.h"

static void notify_scheduler_about_new_operation(sib_data_structure* sib);

static scheduler_item* create_scheduler_item();

static void wait_operation_finish(scheduler_item* sch_item);

static void free_scheduler_item(scheduler_item* sch_item);

static GSList* create_protection_triple_list(char* subject, char* kp_id,
        GSList* property_list);

GSList* dataflow_append_triple_to_list(GSList* input_list, char* subject, char* predicate,
                                       char* object, ssElementType_t object_type);

static char* generate_inner_subscr_id(GHashTable* inner_subs);

static scheduler_item* create_scheduler_item_for_inner_subscr(
    GSList* subscr_triples, char* subscr_id);

static dataflow_subscr_state* create_subscr_state(gchar* subscr_id,
        scheduler_item* sch_item,
        gchar* kp_id,
        DataflowFunc handler);

static void clean_subscr_state(dataflow_subscr_state* subscr_state,
                               GHashTable* inner_subs, GMutex* inner_subs_lock);

static boolean wait_triples_modification(sib_data_structure* sib, char* subscr_id,
        scheduler_item* sch_item);

/*
 * Operate at a separate thread and check the subscription triples on changes.
 * If the changes are found then the handler function will be called.
 */
static gpointer inner_subscription(gpointer data)
{
    dataflow_inner_subscr* inner_sub = data;
    g_mutex_lock(inner_sub->sib->dataflow_inner_subs_lock);
    gchar* subscr_id = generate_inner_subscr_id(inner_sub->sib->dataflow_inner_subs);
    scheduler_item* sch_item = create_scheduler_item_for_inner_subscr(
                                   inner_sub->inner_subscr_triples, subscr_id);
    dataflow_subscr_state* subscr_state = create_subscr_state(
            subscr_id, sch_item, inner_sub->kp_id, inner_sub->handler);
    g_hash_table_insert(inner_sub->sib->dataflow_inner_subs, subscr_id, subscr_state);
    g_mutex_unlock(inner_sub->sib->dataflow_inner_subs_lock);
    g_async_queue_push(inner_sub->sib->query_queue, sch_item);
    notify_scheduler_about_new_operation(inner_sub->sib);
    wait_operation_finish(sch_item);
    do {
        subscr_state = dataflow_find_subscr_state_by_subscr_id(subscr_id,
                       inner_sub->sib->dataflow_inner_subs,
                       inner_sub->sib->dataflow_inner_storage_lock);
        if (subscr_state->status == M3_SUB_STOPPED) {
            break;
        }
        g_async_queue_push(inner_sub->sib->query_queue, sch_item);
        if (wait_triples_modification(inner_sub->sib, subscr_id, sch_item) == FALSE) {
            continue;
        }
        inner_sub->added_triples = sch_item->rsp->added;
        inner_sub->removed_triples = sch_item->rsp->removed;
        (*inner_sub->handler)(inner_sub);
        m3_free_triple_list_simple(&sch_item->rsp->added);
        m3_free_triple_list_simple(&sch_item->rsp->removed);
    } while (TRUE);
    clean_subscr_state(subscr_state, inner_sub->sib->dataflow_inner_subs,
                       inner_sub->sib->dataflow_inner_subs_lock);
    g_free(inner_sub->kp_id);
    g_free(subscr_id);
    g_free(inner_sub);
    return ss_StatusOK;
}

/*
 * Wait until a triples will be modified.
 * Return true if the subscription triples will be modified or false if
 * a other triples will be modified.
 */
static boolean wait_triples_modification(sib_data_structure* sib, char* subscr_id,
        scheduler_item* sch_item)
{
    dataflow_subscr_state* subscr_state = dataflow_find_subscr_state_by_subscr_id(
            subscr_id, sib->dataflow_inner_subs, sib->dataflow_inner_storage_lock);
    if (subscr_state->status == M3_SUB_PENDING) {
        notify_scheduler_about_new_operation(sib);
    }
    wait_operation_finish(sch_item);
    if (sch_item->rsp->status == ss_SubscriptionNotInvolved) {
        whiteboard_log_debug("Inner subscribe not involved\n");
        return FALSE;
    }
    whiteboard_log_debug("Got new inner subscription result\n");
    if (
        (sch_item->rsp->status == ss_SubscriptionNotInvolved) ||
        ((sch_item->rsp->added == NULL) && (sch_item->rsp->removed == NULL))
    ) {
        whiteboard_log_debug("Inner subscribe not involved, nothing changed\n");
        return FALSE;
    }
    return TRUE;
}

/*
 * Create and return a scheduler item with the special format for inner subscription.
 * The scheduler item must be freed with help of a free_scheduler_item
 * function where a subscr_triples field also will be freed. This means that
 * the free_scheduler_item function take full responsibility for this field.
 */
static scheduler_item* create_scheduler_item_for_inner_subscr(
    GSList* subscr_triples, char* subscr_id)
{
    scheduler_item* sch_item = create_scheduler_item();
    sch_item->header->tr_type = M3_INNER_SUBSCRIBE;
    sch_item->req->type = QueryTypeTemplate;
    sch_item->req->template_query = subscr_triples;
    sch_item->rsp->sub_id = g_strdup(subscr_id);
    sch_item->rsp->first_subscribe = TRUE;
    return sch_item;
}

/*
 * Create and return a dataflow subscription state.
 * The returned variable must be freed with help of a clean_subscr_state function.
 */
static dataflow_subscr_state* create_subscr_state(gchar* subscr_id,
        scheduler_item* sch_item,
        gchar* kp_id,
        DataflowFunc handler)
{
    dataflow_subscr_state* subscr_state = g_new0(dataflow_subscr_state, 1);
    subscr_state->status = M3_SUB_ONGOING;
    subscr_state->subscr_id = g_strdup(subscr_id);
    subscr_state->kp_id = g_strdup(kp_id);
    subscr_state->scheduler_item = sch_item;
    subscr_state->unsubscr_cond = NULL;
    subscr_state->unsubscr = FALSE;
    subscr_state->subscr_function = handler;
    return subscr_state;
}

/*
 * Clean a subscription state variable. If a unsubscr_cond field is not NULL then
 * the condition will indicate as completed for a further inner unsubscribe process.
 */
static void clean_subscr_state(dataflow_subscr_state* subscr_state,
                               GHashTable* inner_subs, GMutex* inner_subs_lock)
{
    free_scheduler_item(subscr_state->scheduler_item);
    g_mutex_lock(inner_subs_lock);
    g_hash_table_remove(inner_subs, subscr_state->subscr_id);
    g_free(subscr_state->subscr_id);
    g_free(subscr_state->kp_id);
    g_mutex_unlock(inner_subs_lock);
    subscr_state->unsubscr = TRUE;
    if (subscr_state->unsubscr_cond) {
        g_cond_signal(subscr_state->unsubscr_cond);
    }
}

/*
 * Generate and return a new subscription id.
 * The id must be freed with help of a g_free() function.
 */
static char* generate_inner_subscr_id(GHashTable* inner_subs)
{
    static gint last_inner_subscr_id = 1;
    gchar* temp_subscr_id = g_strdup_printf("inner_subs_%d", ++last_inner_subscr_id);
    while (g_hash_table_lookup(inner_subs, temp_subscr_id)) {
        g_free(temp_subscr_id);
        temp_subscr_id = g_strdup_printf("inner_subs_%d", ++last_inner_subscr_id);
    }
    return temp_subscr_id;
}

/* If a state of a inner subscription is not equal of stop state then
 * set the inner subscription to pending. */
void dataflow_set_inner_subscr_to_pending(gpointer subscr_id, gpointer subscr_data,
        gpointer unused)
{
    dataflow_subscr_state* subscr_state = subscr_data;
    if (subscr_state->status != M3_SUB_STOPPED) {
        subscr_state->status = M3_SUB_PENDING;
    }
    whiteboard_log_debug("Set inner subscription %s to pending\n", subscr_state->subscr_id);
}

/* Find triple in input_list by predicate and object. */
ssTriple_t* dataflow_find_triple_by_predicate_and_object(GSList* input_list,
        char* predicate, char* object)
{
    while (input_list != NULL) {
        ssTriple_t* triple = input_list->data;
        if (g_strcmp0(triple->predicate, predicate) == 0
                && g_strcmp0(triple->object, object) == 0) {
            return triple;
        }
        input_list = input_list->next;
    }
    return NULL;
}

/* Find triple in input_list by predicate and object. */
ssTriple_t* dataflow_find_triple_by_predicate(GSList* input_list, char* predicate)
{
    while (input_list != NULL) {
        ssTriple_t* triple = input_list->data;
        if (g_strcmp0(triple->predicate, predicate) == 0) {
            return triple;
        }
        input_list = input_list->next;
    }
    return NULL;
}

/* A signal for the scheduler that new a operation has been added. */
static void notify_scheduler_about_new_operation(sib_data_structure* sib)
{
    g_mutex_lock(sib->new_reqs_lock);
    sib->new_reqs = TRUE;
    g_cond_signal(sib->new_reqs_cond);
    g_mutex_unlock(sib->new_reqs_lock);
}

/*
 * Create copy of a input list. A returned list must be deleted
 * with help a m3_free_triple_list_simple function.
 */
GSList* dataflow_copy_triple_list(GSList* input_triple_list)
{
    GSList* output_triple_list = NULL;
    ssTriple_t* triple;
    while (input_triple_list) {
        ssCopyTriple(input_triple_list->data, &triple);
        output_triple_list = g_slist_append(output_triple_list, triple);
        input_triple_list = input_triple_list->next;
    }
    return output_triple_list;
}

/*
 * Create a triple with specified a subject, predicate, object and object_type
 * parameters and a default subject_type parameter that equal a ssElement_TYPE_URI value.
 */
ssTriple_t* dataflow_create_triple(char* subject, char* predicate,
                                   char* object, ssElementType_t obj_type)
{
    ssTriple_t* triple = g_new0(ssTriple_t, 1);
    triple->subject = subject;
    triple->predicate = predicate;
    triple->object = object;
    triple->subjType = ssElement_TYPE_URI;
    triple->objType = obj_type;
    return triple;
}

/* Clean all fields in a scheduler_item instance and free this instance. */
static void free_scheduler_item(scheduler_item* sch_item)
{
    if (sch_item->req->insert_graph) {
        m3_free_triple_list_simple(&sch_item->req->insert_graph, NULL);
    }
    if (sch_item->req->remove_graph) {
        m3_free_triple_list_simple(&sch_item->req->remove_graph, NULL);
    }
    if (sch_item->req->template_query) {
        m3_free_triple_list_simple(&sch_item->req->template_query, NULL);
    }
    if (sch_item->rsp->results) {
        m3_free_triple_list_simple(&sch_item->rsp->results, NULL);
    }
    if (sch_item->rsp->sub_id) {
        g_free(sch_item->rsp->sub_id);
    }
    if (sch_item->rsp->added) {
        m3_free_triple_list_simple(&sch_item->rsp->added, NULL);
    }
    if (sch_item->rsp->removed) {
        m3_free_triple_list_simple(&sch_item->rsp->removed, NULL);
    }
    g_free(sch_item->header->kp_id);
    g_cond_free(sch_item->op_cond);
    g_mutex_free(sch_item->op_lock);
    g_free(sch_item->header);
    g_free(sch_item->req);
    g_free(sch_item->rsp);
    g_free(sch_item);
}

/* Wait complete of a operation that is associated with the scheduler item. */
static void wait_operation_finish(scheduler_item* sch_item)
{
    g_mutex_lock(sch_item->op_lock);
    while (!(sch_item->op_complete)) {
        g_cond_wait(sch_item->op_cond, sch_item->op_lock);
    }
    sch_item->op_complete = FALSE;
    g_mutex_unlock(sch_item->op_lock);
}

/*
 * Create and return a scheduler item. The returned variable must be freed
 * with help of a free_scheduler_item function.
 */
static scheduler_item* create_scheduler_item()
{
    scheduler_item* sch_item = g_new0(scheduler_item, 1);
    sch_item->header = g_new0(ssap_message_header, 1);
    sch_item->header->kp_id = NULL;
    sch_item->req = g_new0(ssap_kp_message, 1);
    sch_item->rsp = g_new0(ssap_sib_message, 1);
    sch_item->req->insert_graph = sch_item->req->remove_graph = NULL;
    sch_item->req->template_query = NULL;
    sch_item->rsp->added = sch_item->rsp->removed = NULL;
    sch_item->rsp->results = NULL;
    sch_item->rsp->sub_id = NULL;
    sch_item->op_lock = g_mutex_new();
    sch_item->op_cond = g_cond_new();
    sch_item->op_complete = FALSE;
    return sch_item;
}

/*
 * Create a inner_subscription structure instance. The insert_triples list must be freed manually
 * with help a m3_free_triple_list_simple function.
 */
void dataflow_create_inner_subscription(sib_data_structure* sib, GSList* insert_triples,
                                        DataflowFunc function, gchar* kp_id)
{
    dataflow_inner_subscr* inner_sub = g_new0(dataflow_inner_subscr, 1);
    inner_sub->inner_subscr_triples = dataflow_copy_triple_list(insert_triples);
    inner_sub->handler = function;
    inner_sub->sib = sib;
    inner_sub->kp_id = g_strdup(kp_id);
    g_thread_create(inner_subscription, inner_sub, FALSE, NULL);
}

/* Send a subscription indication message to a client at a connection. */
void dataflow_send_subscription_indication_message(GSList* insert_triples,
        GSList* removed_triples, gchar* kp_id, dataflow_network_conn_data* network_data)
{
    if (insert_triples || removed_triples) {
        gchar* new_triple_str = m3_gen_triple_string(insert_triples, NULL);
        gchar* obsolete_triple_str = m3_gen_triple_string(removed_triples, NULL);
        if (++(network_data->rsp->ind_seqnum) == SSAP_IND_WRAP_NUM) {
            network_data->rsp->ind_seqnum = 1;
        }
        whiteboard_util_send_signal(SIB_DBUS_OBJECT,
                                    SIB_DBUS_KP_INTERFACE,
                                    SIB_DBUS_KP_SIGNAL_SUBSCRIPTION_IND,
                                    network_data->conn,
                                    DBUS_TYPE_STRING, &network_data->space_id,
                                    DBUS_TYPE_STRING, &kp_id,
                                    DBUS_TYPE_INT32, &network_data->tr_id,
                                    DBUS_TYPE_INT32, &network_data->rsp->ind_seqnum,
                                    DBUS_TYPE_STRING, &network_data->rsp->sub_id,
                                    DBUS_TYPE_STRING, &new_triple_str,
                                    DBUS_TYPE_STRING, &obsolete_triple_str,
                                    WHITEBOARD_UTIL_LIST_END);
    }
}

/*
 * Return a triples that was queried. This function take full responsibility
 * for a triple_list variable. This means that the variable will be freed. A returned
 * triple list must be removed with help of a m3_free_triple_list_simple function.
 */
GSList* dataflow_inner_query(sib_data_structure* sib, GSList* triple_list)
{
    scheduler_item* sch_item = create_scheduler_item();
    sch_item->header->tr_type = M3_QUERY;
    sch_item->req->type = QueryTypeTemplate;
    sch_item->req->template_query = triple_list;
    g_async_queue_push(sib->query_queue, sch_item);
    notify_scheduler_about_new_operation(sib);
    wait_operation_finish(sch_item);
    GSList* result = dataflow_copy_triple_list(sch_item->rsp->results);
    free_scheduler_item(sch_item);
    return result;
}

/*
 * Update a triples in the triple storage. This function take full responsibility
 * for a insert_triples and removed_triples variables. This means that
 * the variables will be freed.
 */
void dataflow_inner_update(sib_data_structure* sib, GSList* insert_triples,
                           GSList* removed_triples, char* kp_id)
{
    scheduler_item* sch_item = create_scheduler_item();
    sch_item->header->tr_type = M3_UPDATE;
    sch_item->header->tr_id = DATAFLOW_INNER_OPERATION_TRANSACTION_ID;
    sch_item->header->kp_id = g_strdup(kp_id);
    sch_item->req->encoding = EncodingM3XML;
    sch_item->req->insert_graph = insert_triples;
    sch_item->req->remove_graph = removed_triples;
    g_async_queue_push(sib->insert_queue, sch_item);
    notify_scheduler_about_new_operation(sib);
    wait_operation_finish(sch_item);
    free_scheduler_item(sch_item);
}

/*
 * Remove protection from triples. Input property list must be freed by manually.
 */
void dataflow_remove_triple_protection(sib_data_structure* sib, char* subject,
                                       char* kp_id, GSList* property_list)
{
    dataflow_inner_update(sib, NULL,
                          create_protection_triple_list(subject, kp_id, property_list), kp_id);
}

/*
 * Setup protection from triples. Input property list must be freed by manually.
 */
void dataflow_setup_triple_protection(sib_data_structure* sib, char* subject,
                                      char* kp_id, GSList* property_list)
{
    dataflow_inner_update(sib, create_protection_triple_list(subject, kp_id, property_list),
                          NULL, kp_id);
}

/* Create triple list for triple protection operations. */
static GSList* create_protection_triple_list(char* subject, char* kp_id,
        GSList* property_list)
{
    char* temp_id = g_strdup_printf("%s%i", DATAFLOW_PROTECTION_ID_PREFIX,
                                    g_random_int_range(100, 100000));
    GSList* result_list = NULL;
    while (property_list != NULL) {
        result_list = dataflow_append_triple_to_list(result_list, temp_id, AR_TARGET,
                      property_list->data, ssElement_TYPE_LIT);
        property_list = property_list->next;
    }
    result_list = dataflow_append_triple_to_list(result_list, temp_id, AR_OWNER,
                  kp_id, ssElement_TYPE_URI);
    result_list = dataflow_append_triple_to_list(result_list, subject, AR_PROPERTY,
                  temp_id, ssElement_TYPE_URI);
    return result_list;
}

/* Create triple by subject, predicate, object and object_type and append the triple to list. */
GSList* dataflow_append_triple_to_list(GSList* input_list, char* subject, char* predicate,
                                       char* object, ssElementType_t object_type)
{
    return g_slist_append(input_list, dataflow_create_triple(
                              g_strdup(subject),
                              g_strdup(predicate),
                              g_strdup(object),
                              object_type));
}

/*
 * Stop a inner subscription. If a is_need_to_wait variable is equal TRUE then
 * this function will wait closing of a thread that associated with the subscription.
 */
void dataflow_stop_inner_subscription(sib_data_structure* sib, gchar* kp_id,
                                      DataflowFunc function, boolean is_need_to_wait)
{
    g_mutex_lock(sib->dataflow_inner_subs_lock);
    dataflow_subscr_state* subscr_state = dataflow_find_subscr_state_by_function_and_kp_id(
            function, kp_id, sib->dataflow_inner_subs);
    if (is_need_to_wait == TRUE) {
        subscr_state->unsubscr = FALSE;
        subscr_state->unsubscr_cond = g_cond_new();
    }
    subscr_state->status = M3_SUB_STOPPED;
    g_mutex_unlock(sib->dataflow_inner_subs_lock);
    notify_scheduler_about_new_operation(sib);
    if (is_need_to_wait == TRUE) {
        g_mutex_lock(sib->dataflow_inner_subs_lock);
        while (!subscr_state->unsubscr) {
            g_cond_wait(subscr_state->unsubscr_cond, sib->dataflow_inner_subs_lock);
        }
        subscr_state->unsubscr = FALSE;
        g_cond_free(subscr_state->unsubscr_cond);
        g_mutex_unlock(sib->dataflow_inner_subs_lock);
    }
}
