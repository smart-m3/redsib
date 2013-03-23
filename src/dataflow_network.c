#include "dataflow_network.h"

#include <whiteboard_log.h>
#include "dataflow_entry_search.h"

static dataflow_subst_agent_entry* find_free_substitute_agent(
    sib_data_structure* sib, char* substitute_program_type);

static void register_main_agent(gpointer data);

static void reassign_agent_state_and_output_owner(sib_data_structure* sib, char* old_owner,
        char* new_owner, char* main_agent_id, dataflow_subst_program* subst_program);

static dataflow_subst_program* extract_substitute_program(char* subst_program_str);

static void handle_active_state_triple_adding(gpointer data);

static void stop_substitution(sib_data_structure* sib, ssTriple_t* message_triple);

static void start_main_agent_operation_after_returning(sib_data_structure* sib,
        dataflow_subst_agent_entry* subst_agent);

static void reassing_main_agent_conf_owner(sib_data_structure* sib, char* agent_id,
        char* old_owner, char* new_owner);

static void remove_agent_from_network(gpointer data);

static void register_substitute_agent(gpointer data);

static void update_agent_status_triple_to_inactive(sib_data_structure* sib,
        char* agent_id, char* kp_id);

static void append_triples_to_inner_storage(GSList* input_triples, boolean is_added,
        dataflow_inner_storage_entry* storage_entry);

static void send_stored_triples(sib_data_structure* sib, char* storage_owner_kp_id,
                                char* dest_agent_kp_id, dataflow_network_conn_data* network_data);

static void stop_substitute_agent_work(sib_data_structure* sib,
                                       dataflow_main_agent_entry* main_agent,
                                       dataflow_subst_agent_entry* subst_agent);

static void try_to_substitute_broken_main_agent(sib_data_structure* sib,
        dataflow_main_agent_entry* broken_main_agent);

static void prepare_main_and_subst_agents_to_substitution(sib_data_structure* sib,
        dataflow_main_agent_entry* broken_main_agent, dataflow_subst_agent_entry* subst_agent);

static void prepare_subst_agents_to_substitution(sib_data_structure* sib,
        dataflow_subst_agent_entry* broken_subst_agent, dataflow_subst_agent_entry* subst_agent);

static void add_program_info_to_main_agent_entry(sib_data_structure* sib,
        dataflow_main_agent_entry* main_agent);

static void change_triples_owner(char* old_owner, char* new_owner, GSList* dataflow_permissions);

static void notify_substitute_agent_about_main_agent_returning(sib_data_structure* sib,
        char* main_agent_id, char* subst_agent_id, char* subst_agent_kp_id);

static void start_main_agent_work_immidiatly(sib_data_structure* sib,
        dataflow_main_agent_entry* main_agent);

static GSList* append_triple_with_active_predicate(char* subject,
        char* object, GSList* input_list);

static GSList* append_triple_with_substitutes_predicate(char* subject,
        char* object, GSList* input_list);

static void notify_substitute_agent_about_start_substitution(sib_data_structure* sib,
        char* main_agent_id, char* subst_agent_id, char* subst_agent_kp_id);

static void free_and_remove_main_agent(sib_data_structure* sib,
                                       dataflow_main_agent_entry* main_agent);

static void free_and_remove_substitute_agent(sib_data_structure* sib,
        dataflow_subst_agent_entry* subst_agent);

static void append_agent_to_agent_list(sib_data_structure* sib, char* object,
                                       char* subject, char* kp_id);

/* A inner subscription handler that add a triples to inner storage. */
static void add_data_to_inner_storage(gpointer data)
{
    dataflow_inner_subscr* inner_subscr = data;
    g_mutex_lock(inner_subscr->sib->dataflow_inner_storage_lock);
    GSList* storage_entry_list = dataflow_find_storage_entries_by_kp_id(inner_subscr->kp_id,
                                 inner_subscr->sib->dataflow_inner_storage);
    dataflow_inner_storage_entry* storage_entry;
    if (storage_entry_list == NULL) {
        storage_entry = g_new0(dataflow_inner_storage_entry, 1);
        storage_entry->kp_id = g_strdup(inner_subscr->kp_id);
        inner_subscr->sib->dataflow_inner_storage = g_slist_append(
                    inner_subscr->sib->dataflow_inner_storage, storage_entry);
    } else {
        storage_entry = storage_entry_list->data;
    }
    append_triples_to_inner_storage(inner_subscr->removed_triples, FALSE, storage_entry);
    append_triples_to_inner_storage(inner_subscr->added_triples, TRUE, storage_entry);
    g_mutex_unlock(inner_subscr->sib->dataflow_inner_storage_lock);
}

/*
 * Append triples to a inner storage. A is_added field indicate
 * triples status (added or removed).
 */
static void append_triples_to_inner_storage(GSList* input_triples, boolean is_added,
        dataflow_inner_storage_entry* storage_entry)
{
    while (input_triples) {
        ssTriple_t* triple_copy;
        ssCopyTriple(input_triples->data, &triple_copy);
        dataflow_inner_storage_triple* storage_triple = g_new0(dataflow_inner_storage_triple, 1);
        storage_triple->triple = triple_copy;
        storage_triple->is_added = is_added;
        storage_entry->triple_list = g_slist_append(storage_entry->triple_list, storage_triple);
        input_triples = input_triples->next;
    }
}

/*
 * Create a inner subscription for agent registration. A agent_type field may to take
 * one of a following values: DATAFLOW_TYPE_MAIN_AGENT or DATAFLOW_TYPE_SUBSTITUTE_AGENT.
 */
static void subscribe_to_agent_registration(sib_data_structure* sib, char* agent_type,
        DataflowFunc handler)
{
    GSList* inner_subscr_triples = dataflow_append_triple_to_list(NULL, wildcard2,
                                   DATAFLOW_TYPE, agent_type, ssElement_TYPE_URI);
    dataflow_create_inner_subscription(sib, inner_subscr_triples, handler, NULL);
    m3_free_triple_list_simple(&inner_subscr_triples);
}

/*
 * Initialize a dataflow variables. If one of the variables is not
 * created then a exit function will be called with -1 parameter.
 */
void dataflow_initialize_variables(sib_data_structure* sib)
{
    sib->dataflow_inner_storage = NULL;
    sib->dataflow_main_agents = NULL;
    sib->dataflow_subst_agents = NULL;
    sib->dataflow_inner_storage_lock = g_mutex_new();
    if (NULL == sib->dataflow_inner_storage_lock) {
        exit(-1);
    }
    sib->dataflow_operation_lock = g_mutex_new();
    if (NULL == sib->dataflow_operation_lock) {
        exit(-1);
    }
    sib->dataflow_inner_subs = g_hash_table_new(g_str_hash, g_str_equal);
    if (NULL == sib->dataflow_inner_subs) {
        exit(-1);
    }
    sib->dataflow_inner_subs_lock = g_mutex_new();
    if (NULL == sib->dataflow_inner_subs_lock) {
        exit(-1);
    }
}

/* Handle adding of triple with 'Active' predicate and 'no' object. */
static void handle_inactive_state_triple_adding(gpointer data)
{
    dataflow_inner_subscr* inner_subscr = data;
    ssTriple_t* message_triple;
    if (inner_subscr->added_triples == NULL || (message_triple =
                dataflow_find_triple_by_predicate_and_object(inner_subscr->added_triples,
                        DATAFLOW_ACTIVE, DATAFLOW_NO)) == NULL) {
        return;
    }
    char* agent_id = message_triple->subject;
    if (g_strstr_len(agent_id, -1, DATAFLOW_MAIN_AGENT_PREFIX) != NULL) {
        /* Substitute a dataflow agent when the agent signalizes about a low battery level. */
        g_mutex_lock(inner_subscr->sib->dataflow_operation_lock);
        GSList* main_agent_list = dataflow_find_main_agents_by_agent_id(
                                      agent_id, inner_subscr->sib->dataflow_main_agents);
        g_mutex_unlock(inner_subscr->sib->dataflow_operation_lock);
        if (main_agent_list != NULL) {
            dataflow_main_agent_entry* main_agent = main_agent_list->data;
            if (main_agent->is_work == TRUE) {
                dataflow_try_to_substitute_broken_agent(inner_subscr->sib, main_agent->kp_id);
            }
        }
    } else if (g_strstr_len(agent_id, -1, DATAFLOW_SUBSTITUTE_AGENT_PREFIX) != NULL) {
        g_mutex_lock(inner_subscr->sib->dataflow_operation_lock);
        GSList* subst_agent_list = dataflow_find_subst_agents_by_agent_id(
                                       agent_id, inner_subscr->sib->dataflow_subst_agents);
        g_mutex_unlock(inner_subscr->sib->dataflow_operation_lock);
        if (subst_agent_list != NULL) {
            dataflow_subst_agent_entry* subst_agent = subst_agent_list->data;
            if (subst_agent->state == SUBSTITUTE) {
                /* The agent signalizes about a low battery level.*/
                dataflow_try_to_substitute_broken_agent(inner_subscr->sib, subst_agent->kp_id);
            } else if (subst_agent->state == WAIT_FINISH_CONFIRMATION) {
                start_main_agent_operation_after_returning(inner_subscr->sib, subst_agent);
            }
        }
    }
}

/*
 * Create a inner subscriptions for handle of a dataflow agent registration
 * and a voluntary agent quit.
 */
void dataflow_run_network_operation(sib_data_structure* sib)
{
    subscribe_to_agent_registration(sib, DATAFLOW_TYPE_MAIN_AGENT,
                                    register_main_agent);
    subscribe_to_agent_registration(sib, DATAFLOW_TYPE_SUBSTITUTE_AGENT,
                                    register_substitute_agent);
    ssTriple_t* dataflow_no_triples =
        append_triple_with_active_predicate(wildcard2, DATAFLOW_NO, NULL);
    dataflow_create_inner_subscription(sib, dataflow_no_triples,
                                       handle_inactive_state_triple_adding, NULL);
    m3_free_triple_list_simple(&dataflow_no_triples);
    GSList* dataflow_yes_triples =
        append_triple_with_active_predicate(wildcard2, DATAFLOW_YES, NULL);
    dataflow_create_inner_subscription(sib, dataflow_yes_triples,
                                       handle_active_state_triple_adding, NULL);
    m3_free_triple_list_simple(&dataflow_yes_triples);
    GSList* dataflow_type_triples = dataflow_append_triple_to_list(NULL, wildcard2, DATAFLOW_TYPE,
                                    wildcard2, ssElement_TYPE_URI);
    dataflow_create_inner_subscription(sib, dataflow_type_triples,
                                       remove_agent_from_network, NULL);
    m3_free_triple_list_simple(&dataflow_type_triples);
}

/* Save a connection information to a correspond agent entry. */
void dataflow_save_connection_info_about_agent(sib_data_structure* sib,
        ssap_message_header* header,
        GSList* request_triples,
        ssap_sib_message* response,
        DBusConnection* connection)
{
    ssTriple_t* message_triple;
    if ((message_triple = dataflow_find_triple_by_predicate(request_triples,
                          DATAFLOW_CHANNEL)) == NULL) {
        return;
    }
    g_mutex_lock(sib->dataflow_operation_lock);
    GSList* subst_agent_list = dataflow_find_subst_agents_by_agent_id(message_triple->subject,
                               sib->dataflow_subst_agents);
    dataflow_network_conn_data* network_data = g_new0(dataflow_network_conn_data, 1);
    if (subst_agent_list) {
        dataflow_subst_agent_entry* subst_agent = subst_agent_list->data;
        subst_agent->network_data = network_data;
    }
    GSList* main_agent_list = dataflow_find_main_agents_by_agent_id(message_triple->subject,
                              sib->dataflow_main_agents);
    if (main_agent_list) {
        dataflow_main_agent_entry* main_agent = main_agent_list->data;
        main_agent->network_data = network_data;
        main_agent->is_receive_conn = TRUE;
        g_cond_signal(main_agent->receive_conn_cond);
    }
    network_data->conn = connection;
    network_data->space_id = g_strdup(header->space_id);
    network_data->tr_id = header->tr_id;
    network_data->rsp = response;
    if (!subst_agent_list && !main_agent_list) {
        g_free(network_data);
    }
    g_mutex_unlock(sib->dataflow_operation_lock);
}

/* A handler that start an agent substitution. */
static void start_agent_substitution(sib_data_structure* sib, ssTriple_t* message_triple)
{
    /* Stop a inner subscribes for a substitute and broken agents. */
    g_mutex_lock(sib->dataflow_operation_lock);
    dataflow_subst_agent_entry* subst_agent = dataflow_find_subst_agents_by_agent_id(
                message_triple->subject, sib->dataflow_subst_agents)->data;
    dataflow_stop_inner_subscription(sib, subst_agent->substitutes_kp_id,
                                     add_data_to_inner_storage, TRUE);
    send_stored_triples(sib, subst_agent->substitutes_kp_id,
                        subst_agent->kp_id, subst_agent->network_data);
    subst_agent->state = SUBSTITUTE;
    g_cond_signal(subst_agent->substitute_cond);
    g_mutex_unlock(sib->dataflow_operation_lock);
}

/* Register a main agent in a dataflow network. */
static void register_main_agent(gpointer data)
{
    dataflow_inner_subscr* inner_subscr = data;
    ssTriple_t* message_triple;
    if (inner_subscr->added_triples == NULL || (message_triple =
                dataflow_find_triple_by_predicate_and_object(inner_subscr->added_triples,
                        DATAFLOW_TYPE, DATAFLOW_TYPE_MAIN_AGENT)) == NULL
            || g_strstr_len(message_triple->subject, -1, DATAFLOW_MAIN_AGENT_PREFIX) == NULL) {
        return;
    }
    g_mutex_lock(inner_subscr->sib->dataflow_operation_lock);
    dataflow_main_agent_entry* main_agent = dataflow_find_main_agents_by_agent_id(
            message_triple->subject, inner_subscr->sib->dataflow_main_agents)->data;
    main_agent->substitute_program_type = NULL;
    main_agent->substitute_program = NULL;
    main_agent->temp_kp_id = NULL;
    main_agent->work_cond = g_cond_new();
    main_agent->substitute_agent_kp_id = NULL;
    g_mutex_unlock(inner_subscr->sib->dataflow_operation_lock);
    inner_subscr->added_triples = NULL;
    inner_subscr->removed_triples = NULL;
}

/* Find and add a substitute program information to a main agent entry fields. */
static void add_program_info_to_main_agent_entry(sib_data_structure* sib,
        dataflow_main_agent_entry* main_agent)
{
    if (main_agent->substitute_program_type != NULL && main_agent->substitute_program != NULL) {
        return;
    }
    GSList* query_triples = dataflow_append_triple_to_list(NULL, main_agent->agent_id,
                            DATAFLOW_SUBSTITUTE_PROGRAM, wildcard2, ssElement_TYPE_LIT);
    query_triples = dataflow_append_triple_to_list(query_triples, main_agent->agent_id,
                    DATAFLOW_SUBSTITUTE_PROGRAM_TYPE, wildcard2, ssElement_TYPE_LIT);
    GSList* agent_program_triples = dataflow_inner_query(sib, query_triples);
    while (agent_program_triples != NULL) {
        ssTriple_t* triple = agent_program_triples->data;
        if (!g_strcmp0(triple->predicate, DATAFLOW_SUBSTITUTE_PROGRAM_TYPE)) {
            main_agent->substitute_program_type = g_strdup(triple->object);
        } else if (!g_strcmp0(triple->predicate, DATAFLOW_SUBSTITUTE_PROGRAM)) {
            main_agent->substitute_program = extract_substitute_program(triple->object);
        }
        agent_program_triples = agent_program_triples->next;
    }
    ssFreeTripleList(&agent_program_triples);
}

/* Return string copy if the string is not equal "*" character or wildcard2 copy else. */
static char* copy_string_from_program_header(char* string)
{
    if (g_strcmp0(string, "*") == 0) {
        return g_strdup(wildcard2);
    } else {
        return g_strdup(string);
    }
}

/* Extract substitute program from string to structure. */
static dataflow_subst_program* extract_substitute_program(char* subst_program_str)
{
    dataflow_subst_program* subst_program = g_new0(dataflow_subst_program, 1);
    subst_program->results = NULL;
    subst_program->states = NULL;
    subst_program->subscriptions = NULL;
    char* compressed_subst_program_str = g_strcompress(subst_program_str);
    char** splitted_program = g_strsplit(compressed_subst_program_str, "!", 2);
    char** splitted_header = g_strsplit(splitted_program[0], "\n", -1);
    char** splitted_subscrs = g_strsplit(splitted_header[0], " ", -1);
    char** subscr_item;
    for (subscr_item = splitted_subscrs; *subscr_item != NULL; subscr_item++) {
        ssTriple_t* triple = g_new0(ssTriple_t, 1);
        char** splitted_subscr = g_strsplit_set(*subscr_item, ",|", -1);
        triple->subject = copy_string_from_program_header(splitted_subscr[0]);
        triple->predicate = copy_string_from_program_header(splitted_subscr[1]);
        triple->object = copy_string_from_program_header(splitted_subscr[2]);
        triple->subjType = ssElement_TYPE_URI;
        if (g_strcmp0(splitted_subscr[3], "uri") == 0) {
            triple->objType = ssElement_TYPE_URI;
        } else {
            triple->objType = ssElement_TYPE_LIT;
        }
        g_strfreev(splitted_subscr);
        subst_program->subscriptions = g_slist_append(subst_program->subscriptions, triple);
    }
    g_strfreev(splitted_subscrs);
    char** splitted_results = g_strsplit(splitted_header[1], " ", -1);
    char** result_item;
    for (result_item = splitted_results; *result_item != NULL; result_item++) {
        subst_program->results = g_slist_append(subst_program->results, g_strdup(*result_item));
    }
    g_strfreev(splitted_results);
    char** splitted_states = g_strsplit(splitted_header[2], " ", -1);
    char** state_item;
    for (state_item = splitted_states; *state_item != NULL; state_item++) {
        subst_program->states = g_slist_append(subst_program->states, g_strdup(*state_item));
    }
    g_strfreev(splitted_states);
    g_strfreev(splitted_header);
    g_strfreev(splitted_program);
    g_free(compressed_subst_program_str);
    return subst_program;
}

/* Register a substitute agent in a dataflow network. */
static void register_substitute_agent(gpointer data)
{
    dataflow_inner_subscr* inner_subscr = data;
    ssTriple_t* message_triple;
    if (inner_subscr->added_triples == NULL || (message_triple =
                dataflow_find_triple_by_predicate_and_object(inner_subscr->added_triples,
                        DATAFLOW_TYPE, DATAFLOW_TYPE_SUBSTITUTE_AGENT)) == NULL || g_strstr_len(
                message_triple->subject, -1, DATAFLOW_SUBSTITUTE_AGENT_PREFIX) == NULL) {
        return;
    }
    g_mutex_lock(inner_subscr->sib->dataflow_operation_lock);
    dataflow_subst_agent_entry* subst_agent = dataflow_find_subst_agents_by_agent_id(
                message_triple->subject, inner_subscr->sib->dataflow_subst_agents)->data;
    subst_agent->state = REST;
    subst_agent->substitute_cond = g_cond_new();
    subst_agent->substitute_program_type = NULL;
    subst_agent->substitutes_kp_id = NULL;
    GSList* query_triples = dataflow_append_triple_to_list(NULL, subst_agent->agent_id,
                            DATAFLOW_SUBSTITUTE_PROGRAM_TYPE, wildcard2, ssElement_TYPE_LIT);
    GSList* subst_program_triples = dataflow_inner_query(inner_subscr->sib, query_triples);
    subst_agent->substitute_program_type =
        g_strdup(((ssTriple_t*) subst_program_triples->data)->object);
    g_mutex_unlock(inner_subscr->sib->dataflow_operation_lock);
    inner_subscr->added_triples = NULL;
    inner_subscr->removed_triples = NULL;
    ssFreeTripleList(&subst_program_triples);
}

/* Start a main agent operation after returning. */
static void start_main_agent_operation_after_returning(sib_data_structure* sib,
        dataflow_subst_agent_entry* subst_agent)
{
    /* Stop a inner subscribes for a substitute agent. */
    g_mutex_lock(sib->dataflow_operation_lock);
    subst_agent->state = REST;
    dataflow_stop_inner_subscription(sib, subst_agent->kp_id, add_data_to_inner_storage, TRUE);
    dataflow_main_agent_entry* main_agent = dataflow_find_main_agents_by_kp_id(
            subst_agent->substitutes_kp_id, sib->dataflow_main_agents)->data;
    reassign_agent_state_and_output_owner(sib, subst_agent->kp_id, main_agent->kp_id,
                                          main_agent->agent_id, main_agent->substitute_program);
    send_stored_triples(sib, subst_agent->substitutes_kp_id,
                        main_agent->kp_id, main_agent->network_data);
    g_free(main_agent->substitute_agent_kp_id);
    main_agent->substitute_agent_kp_id = NULL;
    main_agent->is_work = TRUE;
    g_cond_signal(main_agent->work_cond);
    g_free(subst_agent->substitutes_kp_id);
    subst_agent->substitutes_kp_id = NULL;
    g_mutex_unlock(sib->dataflow_operation_lock);
}

/*
 * Send stored triples to agent by added/removed triples pairs from an inner storage entry.
 * The entry will be removed from an inner storage list and freed.
 */
static void send_stored_triples(sib_data_structure* sib, char* storage_owner_kp_id,
                                char* dest_agent_kp_id, dataflow_network_conn_data* network_data)
{
    g_mutex_lock(sib->dataflow_inner_storage_lock);
    GSList* inner_storage =
        dataflow_find_storage_entries_by_kp_id(storage_owner_kp_id, sib->dataflow_inner_storage);
    if (inner_storage) {
        GSList* added_triples = NULL;
        GSList* removed_triples = NULL;
        boolean is_added_triples_was_added = FALSE;
        boolean is_removed_triples_was_added = FALSE;
        dataflow_inner_storage_entry* storage_entry = inner_storage->data;
        GSList* storage_triple_list = storage_entry->triple_list;
        while (storage_triple_list) {
            dataflow_inner_storage_triple* storage_triple = storage_triple_list->data;
            if (storage_triple->is_added) {
                if (is_added_triples_was_added && is_removed_triples_was_added) {
                    is_removed_triples_was_added = FALSE;
                    dataflow_send_subscription_indication_message(
                        added_triples, removed_triples, dest_agent_kp_id, network_data);
                    added_triples = NULL;
                    removed_triples = NULL;
                }
                added_triples = g_slist_append(added_triples, storage_triple->triple);
                is_added_triples_was_added = TRUE;
            } else {
                removed_triples = g_slist_append(removed_triples, storage_triple->triple);
                is_removed_triples_was_added = TRUE;
            }
            storage_triple_list = g_slist_remove(storage_triple_list, storage_triple);
            g_free(storage_triple);
        }
        sib->dataflow_inner_storage = g_slist_remove(sib->dataflow_inner_storage, storage_entry);
        g_free(storage_entry->kp_id);
        g_free(storage_entry);
    }
    g_mutex_unlock(sib->dataflow_inner_storage_lock);
}

/* A handler that stop substitution. */
static void stop_substitution(sib_data_structure* sib, ssTriple_t* message_triple)
{
    g_mutex_lock(sib->dataflow_operation_lock);
    GSList* main_agent_list = dataflow_find_main_agents_by_agent_id(
                                  message_triple->subject, sib->dataflow_main_agents);
    if (main_agent_list == NULL) {
        g_mutex_unlock(sib->dataflow_operation_lock);
        return;
    }
    dataflow_main_agent_entry* main_agent = main_agent_list->data;
    if (main_agent->is_work == TRUE) {
        g_mutex_unlock(sib->dataflow_operation_lock);
        return;
    }
    if (main_agent->substitute_agent_kp_id == NULL) {
        /* Need for situation when no a substitute agent was found. */
        dataflow_stop_inner_subscription(sib, main_agent->kp_id, add_data_to_inner_storage, TRUE);
    }
    reassing_main_agent_conf_owner(sib, main_agent->agent_id,
                                   main_agent->kp_id, main_agent->temp_kp_id);
    GSList* subst_agent_list = dataflow_find_subst_agents_by_kp_id(
                                   main_agent->substitute_agent_kp_id, sib->dataflow_subst_agents);
    g_mutex_unlock(sib->dataflow_operation_lock);
    if (subst_agent_list) {
        stop_substitute_agent_work(sib, main_agent, subst_agent_list->data);
    } else {
        start_main_agent_work_immidiatly(sib, main_agent);
    }
}

/* Handle adding of triple with 'Active' predicate and 'yes' object. */
static void handle_active_state_triple_adding(gpointer data)
{
    dataflow_inner_subscr* inner_subscr = data;
    ssTriple_t* message_triple;
    if (inner_subscr->added_triples == NULL || (message_triple =
                dataflow_find_triple_by_predicate_and_object(inner_subscr->added_triples,
                        DATAFLOW_ACTIVE, DATAFLOW_YES)) == NULL) {
        return;
    }
    if (g_strstr_len(message_triple->subject, -1, DATAFLOW_MAIN_AGENT_PREFIX) != NULL) {
        stop_substitution(inner_subscr->sib, message_triple);
    } else if (g_strstr_len(message_triple->subject, -1,
                            DATAFLOW_SUBSTITUTE_AGENT_PREFIX) != NULL) {
        start_agent_substitution(inner_subscr->sib, message_triple);
    }
}

/* Reassign owner of main agent configuration triples. */
static void reassing_main_agent_conf_owner(sib_data_structure* sib, char* agent_id,
        char* old_owner, char* new_owner)
{
    GSList* property_list = g_slist_append(NULL, DATAFLOW_DESCRIPTION);
    property_list = g_slist_append(property_list, DATAFLOW_SUBSTITUTE_PROGRAM_TYPE);
    property_list = g_slist_append(property_list, DATAFLOW_SUBSTITUTE_PROGRAM);
    property_list = g_slist_append(property_list, DATAFLOW_TYPE);
    property_list = g_slist_append(property_list, DATAFLOW_ACTIVE);
    dataflow_remove_triple_protection(sib, agent_id, old_owner, property_list);
    dataflow_setup_triple_protection(sib, agent_id, new_owner, property_list);
    g_slist_free(property_list);
}

/* Stop work of a substitute agent that to substitute of a main agent. */
static void stop_substitute_agent_work(sib_data_structure* sib,
                                       dataflow_main_agent_entry* main_agent,
                                       dataflow_subst_agent_entry* subst_agent)
{
    /* Wait until a substitute agent start to work instead of a broken agent. */
    GMutex* dump_mutex = g_mutex_new();
    while (subst_agent->state == REST) {
        g_cond_wait(subst_agent->substitute_cond, dump_mutex);
    }
    g_mutex_free(dump_mutex);
    /* Change subscription destination to inner storage. */
    dataflow_create_inner_subscription(sib, main_agent->substitute_program->subscriptions,
                                       add_data_to_inner_storage, subst_agent->kp_id);
    /* Install new kp_id to agent entry. */
    g_free(main_agent->kp_id);
    main_agent->kp_id = main_agent->temp_kp_id;
    g_free(subst_agent->substitutes_kp_id);
    subst_agent->substitutes_kp_id = g_strdup(main_agent->temp_kp_id);
    subst_agent->state = WAIT_FINISH_CONFIRMATION;
    main_agent->temp_kp_id = NULL;
    notify_substitute_agent_about_main_agent_returning(sib,
            main_agent->agent_id, subst_agent->agent_id, subst_agent->kp_id);
}

/*
 * Called in situation when a substitute agent was finish work before
 * than a main agent return to a dataflow network or when no the substitute agent was found.
 */
static void start_main_agent_work_immidiatly(sib_data_structure* sib,
        dataflow_main_agent_entry* main_agent)
{
    g_mutex_lock(sib->dataflow_inner_storage_lock);
    char* old_id = main_agent->kp_id;
    main_agent->kp_id = main_agent->temp_kp_id;
    main_agent->temp_kp_id = NULL;
    g_mutex_unlock(sib->dataflow_inner_storage_lock);
    GMutex* dump_mutex = g_mutex_new();
    g_mutex_lock(dump_mutex);
    if (main_agent->is_receive_conn == FALSE) {
        g_cond_wait(main_agent->receive_conn_cond, dump_mutex);
    }
    g_mutex_free(dump_mutex);
    /* Send an all triples from a inner storage for the main agent in situation
     * when no the substitute agent was found. */
    send_stored_triples(sib, old_id, main_agent->kp_id, main_agent->network_data);
    g_free(old_id);
    g_mutex_lock(sib->dataflow_inner_storage_lock);
    g_free(main_agent->substitute_agent_kp_id);
    main_agent->substitute_agent_kp_id = NULL;
    main_agent->is_work = TRUE;
    g_cond_signal(main_agent->work_cond);
    g_mutex_unlock(sib->dataflow_inner_storage_lock);
}

/*
 * Send a notification to a substitute agent that he must to finish broken agent
 * substitution and subscribe to confirmation of it.
 */
static void notify_substitute_agent_about_main_agent_returning(sib_data_structure* sib,
        char* main_agent_id, char* subst_agent_id, char* subst_agent_kp_id)
{
    GSList* removed_triples = append_triple_with_substitutes_predicate(
                                  subst_agent_id, main_agent_id, NULL);
    GSList* added_triples = append_triple_with_substitutes_predicate(
                                subst_agent_id, DATAFLOW_NONE, NULL);
    dataflow_inner_update(sib, added_triples, removed_triples, subst_agent_kp_id);
}

/*
 * Append a triple with a DATAFLOW_ACTIVE predicate to a input_list. A subject and object
 * parameters will be copied. If the input_list is NULL then a returned list will be created and
 * must be freed with help of a g_free function.
 */
static GSList* append_triple_with_active_predicate(char* subject, char* object, GSList* input_list)
{
    return dataflow_append_triple_to_list(input_list, subject, DATAFLOW_ACTIVE,
                                          object, ssElement_TYPE_LIT);
}

/*
 * Append a triple with a DATAFLOW_SUBSTITUTES predicate to a input_list. A subject and object
 * parameters will be copied. If the input_list is NULL then a returned list will be created and
 * must be freed with help of a g_free function.
 */
static GSList* append_triple_with_substitutes_predicate(char* subject,
        char* object, GSList* input_list)
{
    return dataflow_append_triple_to_list(input_list, subject, DATAFLOW_SUBSTITUTES,
                                          object, ssElement_TYPE_URI);
}

/* Try to substitute a broken agent. */
void dataflow_try_to_substitute_broken_agent(sib_data_structure* sib, gchar* kp_id)
{
    g_mutex_lock(sib->dataflow_operation_lock);
    GSList* main_agent_list = dataflow_find_main_agents_by_kp_id(kp_id, sib->dataflow_main_agents);
    GSList* subst_agent_list = dataflow_find_subst_agents_by_kp_id(
                                   kp_id, sib->dataflow_subst_agents);
    if (main_agent_list != NULL
            && ((dataflow_main_agent_entry*) main_agent_list->data)->is_work == TRUE) {
        try_to_substitute_broken_main_agent(sib, main_agent_list->data);
    } else if (subst_agent_list != NULL
               && ((dataflow_subst_agent_entry*) subst_agent_list->data)->state == TRUE) {
        dataflow_subst_agent_entry* broken_subst_agent = subst_agent_list->data;
        broken_subst_agent->state = REST;
        dataflow_main_agent_entry* main_agent = dataflow_find_main_agents_by_kp_id(
                broken_subst_agent->substitutes_kp_id, sib->dataflow_main_agents)->data;
        /* Change a subscription destination to a inner storage. */
        dataflow_create_inner_subscription(sib, main_agent->substitute_program->subscriptions,
                                           add_data_to_inner_storage,
                                           broken_subst_agent->substitutes_kp_id);
        update_agent_status_triple_to_inactive(sib, broken_subst_agent->agent_id,
                                               broken_subst_agent->substitutes_kp_id);
        dataflow_subst_agent_entry* subst_agent = find_free_substitute_agent(
                    sib, broken_subst_agent->substitute_program_type);
        if (subst_agent != NULL) {
            prepare_subst_agents_to_substitution(sib, broken_subst_agent, subst_agent);
        }
    }
    g_mutex_unlock(sib->dataflow_operation_lock);
}

/* Reassign agent state and output triples owner. */
static void reassign_agent_state_and_output_owner(sib_data_structure* sib, char* old_owner,
        char* new_owner, char* main_agent_id, dataflow_subst_program* subst_program)
{
    dataflow_remove_triple_protection(sib, main_agent_id, old_owner, subst_program->states);
    dataflow_remove_triple_protection(sib, main_agent_id, old_owner, subst_program->results);
    dataflow_setup_triple_protection(sib, main_agent_id, new_owner, subst_program->states);
    dataflow_setup_triple_protection(sib, main_agent_id, new_owner, subst_program->results);
}

/* Try to substitute a broken main agent by substitute agent. */
static void try_to_substitute_broken_main_agent(sib_data_structure* sib,
        dataflow_main_agent_entry* broken_main_agent)
{
    add_program_info_to_main_agent_entry(sib, broken_main_agent);
    /* Change a subscription destination to a inner storage. */
    dataflow_create_inner_subscription(sib, broken_main_agent->substitute_program->subscriptions,
                                       add_data_to_inner_storage, broken_main_agent->kp_id);
    broken_main_agent->is_receive_conn = FALSE;
    update_agent_status_triple_to_inactive(sib, broken_main_agent->agent_id,
                                           broken_main_agent->kp_id);
    GSList* active_property = g_slist_append(NULL, DATAFLOW_ACTIVE);
    dataflow_remove_triple_protection(sib, broken_main_agent->agent_id, broken_main_agent->kp_id,
                                      active_property);
    g_slist_free(active_property);
    dataflow_subst_agent_entry* subst_agent = find_free_substitute_agent(
                sib, broken_main_agent->substitute_program_type);
    if (subst_agent != NULL) {
        prepare_main_and_subst_agents_to_substitution(sib, broken_main_agent, subst_agent);
    }
    broken_main_agent->is_work = FALSE;
}

/* Prepare a substitute agents to substitution. */
static void prepare_subst_agents_to_substitution(sib_data_structure* sib,
        dataflow_subst_agent_entry* broken_subst_agent, dataflow_subst_agent_entry* subst_agent)
{
    dataflow_main_agent_entry* main_agent = dataflow_find_main_agents_by_kp_id(
            broken_subst_agent->substitutes_kp_id, sib->dataflow_main_agents)->data;
    main_agent->substitute_agent_kp_id = g_strdup(subst_agent->kp_id);
    subst_agent->substitutes_kp_id = broken_subst_agent->substitutes_kp_id;
    broken_subst_agent->substitutes_kp_id = NULL;
    reassign_agent_state_and_output_owner(sib, broken_subst_agent->kp_id, subst_agent->kp_id,
                                          main_agent->agent_id, main_agent->substitute_program);
    free_and_remove_substitute_agent(sib, broken_subst_agent);
    notify_substitute_agent_about_start_substitution(sib, main_agent->agent_id,
            subst_agent->agent_id, subst_agent->kp_id);
}

/* Prepare a main agent and a substitute agent to substitution. */
static void prepare_main_and_subst_agents_to_substitution(sib_data_structure* sib,
        dataflow_main_agent_entry* broken_main_agent, dataflow_subst_agent_entry* subst_agent)
{
    broken_main_agent->substitute_agent_kp_id = g_strdup(subst_agent->kp_id);
    subst_agent->substitutes_kp_id = g_strdup(broken_main_agent->kp_id);
    subst_agent->state = WAIT_START_CONFIRMATION;
    reassign_agent_state_and_output_owner(sib, broken_main_agent->kp_id, subst_agent->kp_id,
                                          broken_main_agent->agent_id,
                                          broken_main_agent->substitute_program);
    notify_substitute_agent_about_start_substitution(sib, broken_main_agent->agent_id,
            subst_agent->agent_id, subst_agent->kp_id);
}

/* Update an object in (dataflow_agent, active, yes) to a 'no' value. */
static void update_agent_status_triple_to_inactive(sib_data_structure* sib,
        char* agent_id, char* kp_id)
{
    GSList* added_triples = append_triple_with_active_predicate(
                                agent_id, DATAFLOW_NO, NULL);
    GSList* removed_triples = append_triple_with_active_predicate(
                                  agent_id, DATAFLOW_YES, NULL);
    dataflow_inner_update(sib, added_triples, removed_triples, kp_id);
}

/*
 * Send a notification to a substitute agent that he must to start broken agent
 * substitution and subscribe to confirmation of it.
 */
static void notify_substitute_agent_about_start_substitution(sib_data_structure* sib,
        char* main_agent_id, char* subst_agent_id, char* subst_agent_kp_id)
{
    GSList* added_triples = append_triple_with_substitutes_predicate(
                                subst_agent_id, main_agent_id, NULL);
    GSList* removed_triples = append_triple_with_substitutes_predicate(
                                  subst_agent_id, DATAFLOW_NONE, NULL);
    dataflow_inner_update(sib, added_triples, removed_triples, subst_agent_kp_id);
}

/*
 * Return a free substitute agent that not substitutes an any agent or NULL value
 * if no the agent was found.
 */
static dataflow_subst_agent_entry* find_free_substitute_agent(
    sib_data_structure* sib, char* substitute_program_type)
{
    GSList* subst_agent_list = dataflow_find_subst_agents_by_program_type(
                                   substitute_program_type, sib->dataflow_subst_agents);
    while (subst_agent_list) {
        dataflow_subst_agent_entry* subst_agent = subst_agent_list->data;
        GSList* query_triples = append_triple_with_substitutes_predicate(
                                    subst_agent->agent_id, DATAFLOW_NONE, NULL);
        GSList* result_triples = dataflow_inner_query(sib, query_triples);
        if (result_triples) {
            return subst_agent;
        }
        subst_agent_list = subst_agent_list->next;
    }
    return NULL;
}

/* Remove agent from network. */
static void remove_agent_from_network(gpointer data)
{
    dataflow_inner_subscr* inner_subscr = data;
    if (inner_subscr->removed_triples == NULL
            || g_strcmp0(((ssTriple_t*)inner_subscr->removed_triples->data)->predicate,
                         DATAFLOW_TYPE) != 0) {
        return;
    }
    sib_data_structure* sib = inner_subscr->sib;
    ssTriple_t* triple = (ssTriple_t*) inner_subscr->removed_triples->data;
    g_mutex_lock(sib->dataflow_operation_lock);
    if (!g_strcmp0(triple->object, DATAFLOW_TYPE_MAIN_AGENT)) {
        GSList* main_agent_list = dataflow_find_main_agents_by_agent_id(triple->subject,
                                  sib->dataflow_main_agents);
        if (main_agent_list != NULL) {
            free_and_remove_main_agent(sib, main_agent_list->data);
        }
    } else if (!g_strcmp0(triple->object, DATAFLOW_TYPE_SUBSTITUTE_AGENT)) {
        GSList* subst_agent_list = dataflow_find_subst_agents_by_agent_id(triple->subject,
                                   sib->dataflow_subst_agents);
        if (subst_agent_list != NULL) {
            free_and_remove_substitute_agent(sib, subst_agent_list->data);
        }
    }
    g_mutex_unlock(sib->dataflow_operation_lock);
}

/*
 * Free an all fields in a substitute agent entry and
 * remove the entry from a substitute agent list.
 */
static void free_and_remove_substitute_agent(sib_data_structure* sib,
        dataflow_subst_agent_entry* subst_agent)
{
    if (subst_agent->substitutes_kp_id != NULL) {
        g_mutex_unlock(sib->dataflow_operation_lock);
        GMutex* dump_mutex = g_mutex_new();
        g_mutex_lock(dump_mutex);
        while (subst_agent->state == REST) {
            g_cond_wait(subst_agent->substitute_cond, dump_mutex);
        }
        g_mutex_free(dump_mutex);
        g_mutex_lock(sib->dataflow_operation_lock);
    }
    g_free(subst_agent->agent_id);
    g_free(subst_agent->kp_id);
    g_free(subst_agent->substitute_program_type);
    g_free(subst_agent->substitutes_kp_id);
    g_cond_free(subst_agent->substitute_cond);
    if (subst_agent->network_data != NULL) {
        g_free(subst_agent->network_data->space_id);
    }
    sib->dataflow_subst_agents = g_slist_remove(sib->dataflow_subst_agents, subst_agent);
    g_free(subst_agent);
}

/*
 * Free an all fields in a main agent entry and
 * remove the entry from a main agent list.
 */
static void free_and_remove_main_agent(sib_data_structure* sib,
                                       dataflow_main_agent_entry* main_agent)
{
    GMutex* dump_mutex = g_mutex_new();
    g_mutex_unlock(sib->dataflow_operation_lock);
    g_mutex_lock(dump_mutex);
    while (main_agent->is_work == FALSE) {
        g_cond_wait(main_agent->work_cond, dump_mutex);
    }
    g_mutex_free(dump_mutex);
    g_mutex_lock(sib->dataflow_operation_lock);
    g_free(main_agent->agent_id);
    g_free(main_agent->kp_id);
    if (main_agent->substitute_program != NULL) {
        m3_free_triple_list_simple(&main_agent->substitute_program->subscriptions, NULL);
        GSList* results = main_agent->substitute_program->results;
        while (results != NULL) {
            g_free(results->data);
            results = results->next;
        }
        g_slist_free(main_agent->substitute_program->results);
        GSList* states = main_agent->substitute_program->states;
        while (states != NULL) {
            g_free(states->data);
            states = states->next;
        }
        g_slist_free(main_agent->substitute_program->states);
        g_free(main_agent->substitute_program);
    }
    g_free(main_agent->substitute_program_type);
    g_cond_free(main_agent->work_cond);
    if (main_agent->network_data->space_id != NULL) {
        g_free(main_agent->network_data->space_id);
    }
    g_cond_free(main_agent->receive_conn_cond);
    sib->dataflow_main_agents = g_slist_remove(sib->dataflow_main_agents, main_agent);
    g_free(main_agent);
}

/* Try to add a dataflow agent. */
void dataflow_try_to_add_agent(sib_data_structure* sib, GSList* added_triples, char* kp_id)
{
    while (added_triples) {
        ssTriple_t* triple = added_triples->data;
        if (!g_strcmp0(triple->predicate, DATAFLOW_TYPE)) {
            append_agent_to_agent_list(sib, triple->object, triple->subject, kp_id);
        } else if (!g_strcmp0(triple->predicate, DATAFLOW_ACTIVE)
                   && !g_strcmp0(triple->object, DATAFLOW_YES)) {
            g_mutex_lock(sib->dataflow_operation_lock);
            GSList* main_agent_list = dataflow_find_main_agents_by_agent_id(triple->subject,
                                      sib->dataflow_main_agents);
            if (main_agent_list) {
                dataflow_main_agent_entry* main_agent = main_agent_list->data;
                main_agent->temp_kp_id = g_strdup(kp_id);
            }
            g_mutex_unlock(sib->dataflow_operation_lock);
        }
        added_triples = added_triples->next;
    }
}

/*
 * Append an agent entry to a correspond agent list by a subject parameter. The subject
 * parameter may take one of a following values:
 * DATAFLOW_TYPE_MAIN_AGENT or DATAFLOW_TYPE_SUBSTITUTE_AGENT.
 */
static void append_agent_to_agent_list(sib_data_structure* sib, char* object,
                                       char* subject, char* kp_id)
{
    if (!g_strcmp0(object, DATAFLOW_TYPE_MAIN_AGENT)) {
        g_mutex_lock(sib->dataflow_operation_lock);
        GSList* main_agent_list = dataflow_find_main_agents_by_agent_id(subject,
                                  sib->dataflow_main_agents);
        if (main_agent_list == NULL) {
            dataflow_main_agent_entry* main_agent = g_new0(dataflow_main_agent_entry, 1);
            main_agent->agent_id = g_strdup(subject);
            main_agent->kp_id = g_strdup(kp_id);
            main_agent->is_work = TRUE;
            main_agent->is_receive_conn = FALSE;
            main_agent->receive_conn_cond = g_cond_new();
            sib->dataflow_main_agents = g_slist_append(sib->dataflow_main_agents, main_agent);
        }
        g_mutex_unlock(sib->dataflow_operation_lock);
    } else if (!g_strcmp0(object, DATAFLOW_TYPE_SUBSTITUTE_AGENT)) {
        g_mutex_lock(sib->dataflow_operation_lock);
        GSList* subst_agent_list = dataflow_find_subst_agents_by_agent_id(subject,
                                   sib->dataflow_subst_agents);
        if (subst_agent_list == NULL) {
            dataflow_subst_agent_entry* subst_agent =
                g_new0(dataflow_subst_agent_entry, 1);
            subst_agent->agent_id = g_strdup(subject);
            subst_agent->kp_id = g_strdup(kp_id);
            sib->dataflow_subst_agents =
                g_slist_append(sib->dataflow_subst_agents, subst_agent);
        }
        g_mutex_unlock(sib->dataflow_operation_lock);
    }
}
