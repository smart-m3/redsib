#include "dataflow_entry_search.h"
#include "dataflow_network.h"

/*
 * Return a result of g_strcmp0 function calling for a agent_id fields
 * of a dataflow_main_agent_entry structure or -1 if one of a parameters is NULL.
 */
static gint compare_dataflow_agent_entry_by_agent_id(dataflow_main_agent_entry* first,
        dataflow_main_agent_entry* second)
{
    if (first == NULL || second == NULL) {
        return -1;
    }
    return g_strcmp0(first->agent_id, second->agent_id);
}

/*
 * Return a result of g_strcmp0 function calling for a kp_id fields
 * of a dataflow_main_agent_entry structure or -1 if one of a parameters is NULL.
 */
static gint compare_dataflow_agent_entry_by_kp_id(dataflow_main_agent_entry* first,
        dataflow_main_agent_entry* second)
{
    if (first == NULL || second == NULL) {
        return -1;
    }
    return g_strcmp0(first->kp_id, second->kp_id);
}

/*
 * Return a result of g_strcmp0 function calling for a kp_id fields
 * of a dataflow_inner_storage_entry structure or -1 if one of a parameters is NULL.
 */
static gint compare_inner_storage_entry_by_kp_id(dataflow_inner_storage_entry* first,
        dataflow_inner_storage_entry* second)
{
    if (first == NULL || second == NULL) {
        return -1;
    }
    return g_strcmp0(first->kp_id, second->kp_id);
}

/*
 * Return a result of g_strcmp0 function calling for a agent_id fields
 * of a dataflow_substitute_agent_entry structure or -1 if one of a parameters is NULL.
 */
static gint compare_substitute_agent_entry_by_agent_id(dataflow_subst_agent_entry* first,
        dataflow_subst_agent_entry* second)
{
    if (first == NULL || second == NULL) {
        return -1;
    }
    return g_strcmp0(first->agent_id, second->agent_id);
}

/*
 * Return a result of g_strcmp0 function calling for a kp_id fields
 * of a dataflow_substitute_agent_entry structure or -1 if one of a parameters is NULL.
 */
static gint compare_substitute_agent_entry_by_kp_id(dataflow_subst_agent_entry* first,
        dataflow_subst_agent_entry* second)
{
    if (first == NULL || second == NULL) {
        return -1;
    }
    return g_strcmp0(first->kp_id, second->kp_id);
}

/*
 * Return a result of g_strcmp0 function calling for a substitute_program_type fields
 * of a dataflow_substitute_agent_entry structure or -1 if one of a parameters is NULL.
 */
static gint compare_substitute_agent_entry_by_program_type(dataflow_subst_agent_entry* first,
        dataflow_subst_agent_entry* second)
{
    if (first == NULL || second == NULL) {
        return -1;
    }
    return g_strcmp0(first->substitute_program_type, second->substitute_program_type);
}

/*
 * Return a result of g_strcmp0 function calling for a substitute_agent_kp_id fields
 * of a dataflow_main_agent_entry structure or -1 if one of a parameters is NULL.
 */
static gint compare_dataflow_agent_entry_by_substitute_agent_kp_id(
    dataflow_main_agent_entry* first, dataflow_main_agent_entry* second)
{
    if (first == NULL || second == NULL) {
        return -1;
    }
    return g_strcmp0(first->substitute_agent_kp_id, second->substitute_agent_kp_id);
}

/*
 * Return a sublist of elements from a dataflow_inner_storage list
 * that contain a same value in kp_id field or the NULL value if the elements is not found.
 */
GSList* dataflow_find_storage_entries_by_kp_id(gchar* kp_id, GSList* dataflow_inner_storage)
{
    dataflow_inner_storage_entry* dump_storage_entry = g_new0(dataflow_inner_storage_entry, 1);
    dump_storage_entry->kp_id = kp_id;
    GSList* result = g_slist_find_custom(dataflow_inner_storage,
                                         dump_storage_entry,
                                         compare_inner_storage_entry_by_kp_id);
    g_free(dump_storage_entry);
    return result;
}

/*
 * Return a sublist of elements from a dataflow_subst_agents list
 * that contain a same value in kp_id field or the NULL value if the elements is not found.
 */
GSList* dataflow_find_subst_agents_by_kp_id(gchar* kp_id, GSList* dataflow_subst_agents)
{
    dataflow_subst_agent_entry* dump_subst_agent = g_new0(dataflow_subst_agent_entry, 1);
    dump_subst_agent->kp_id = kp_id;
    GSList* result = g_slist_find_custom(dataflow_subst_agents,
                                         dump_subst_agent,
                                         compare_substitute_agent_entry_by_kp_id);
    g_free(dump_subst_agent);
    return result;
}

/*
 * Return a sublist of elements from a dataflow_subst_agents list
 * that contain a same value in agent_id field or the NULL value if the elements is not found.
 */
GSList* dataflow_find_subst_agents_by_agent_id(gchar* agent_id,
        GSList* dataflow_subst_agents)
{
    dataflow_subst_agent_entry* dump_agent = g_new0(dataflow_subst_agent_entry, 1);
    dump_agent->agent_id = agent_id;
    GSList* result = g_slist_find_custom(dataflow_subst_agents,
                                         dump_agent,
                                         compare_substitute_agent_entry_by_agent_id);
    g_free(dump_agent);
    return result;
}

/*
 * Return a sublist of elements from a dataflow_main_agents list
 * that contain a same value in substitute_agent_kp_id field
 * or the NULL value if the elements is not found.
 */
GSList* dataflow_find_main_agents_by_substitute_agent_kp_id(gchar* substitute_agent_kp_id,
        GSList* dataflow_main_agents)
{
    dataflow_main_agent_entry* dump_agent = g_new0(dataflow_main_agent_entry, 1);
    dump_agent->substitute_agent_kp_id = substitute_agent_kp_id;
    GSList* result = g_slist_find_custom(dataflow_main_agents,
                                         dump_agent,
                                         compare_dataflow_agent_entry_by_substitute_agent_kp_id);
    g_free(dump_agent);
    return result;
}

/*
 * Return a sublist of elements from a dataflow_subst_agents list
 * that contain a same value in program_type field or the NULL value if the elements is not found.
 */
GSList* dataflow_find_subst_agents_by_program_type(gchar* program_type,
        GSList* dataflow_subst_agents)
{
    dataflow_subst_agent_entry* dump_subst_agent = g_new0(dataflow_subst_agent_entry, 1);
    dump_subst_agent->substitute_program_type = program_type;
    GSList* result = g_slist_find_custom(dataflow_subst_agents,
                                         dump_subst_agent,
                                         compare_substitute_agent_entry_by_program_type);
    g_free(dump_subst_agent);
    return result;
}

/*
 * Return a sublist of elements from a dataflow_main_agents list
 * that contain a same value in agent_id field or the NULL value if the elements is not found.
 */
GSList* dataflow_find_main_agents_by_agent_id(gchar* agent_id, GSList* dataflow_main_agents)
{
    dataflow_main_agent_entry* dump_agent = g_new0(dataflow_main_agent_entry, 1);
    dump_agent->agent_id = agent_id;
    GSList* result = g_slist_find_custom(dataflow_main_agents,
                                         dump_agent,
                                         compare_dataflow_agent_entry_by_agent_id);
    g_free(dump_agent);
    return result;
}

/*
 * Return a sublist of elements from a dataflow_main_agents list
 * that contain a same value in kp_id field or the NULL value if the elements is not found.
 */
GSList* dataflow_find_main_agents_by_kp_id(gchar* kp_id, GSList* dataflow_main_agents)
{
    dataflow_main_agent_entry* dump_agent = g_new0(dataflow_main_agent_entry, 1);
    dump_agent->kp_id = kp_id;
    GSList* result = g_slist_find_custom(dataflow_main_agents,
                                         dump_agent,
                                         compare_dataflow_agent_entry_by_kp_id);
    g_free(dump_agent);
    return result;
}

/*
 * Return a result of element searching by a subscr_id value in a dataflow_inner_subs
 * hash table with using a dataflow_inner_subs_lock variable for mutex mechanism or
 * the NULL value if the elements is not found.
 */
dataflow_subscr_state* dataflow_find_subscr_state_by_subscr_id(char* subscr_id,
        GHashTable* dataflow_inner_subs, GMutex* dataflow_inner_subs_lock)
{
    g_mutex_lock(dataflow_inner_subs_lock);
    dataflow_subscr_state* subscr_state =
        (dataflow_subscr_state*)g_hash_table_lookup(dataflow_inner_subs, subscr_id);
    g_mutex_unlock(dataflow_inner_subs_lock);
    return subscr_state;
}

/*
 * Return a result of element searching by a kp_id and function values
 * in a dataflow_inner_subs hash table or the NULL value if the elements is not found.
 */
dataflow_subscr_state* dataflow_find_subscr_state_by_function_and_kp_id(
    DataflowFunc function, char* kp_id, GHashTable* dataflow_inner_subs)
{
    GHashTableIter iter;
    gpointer key, value;
    g_hash_table_iter_init(&iter, dataflow_inner_subs);
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        dataflow_subscr_state* sub_state = (dataflow_subscr_state*) value;
        if (sub_state && (sub_state->subscr_function == function)
                && (g_strcmp0(sub_state->kp_id, kp_id) == 0)) {
            return sub_state;
        }
    }
    return NULL;
}
