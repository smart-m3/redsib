/*
 * This file contains function for searching of a data in a GLib containers.
 */

#ifndef DATAFLOW_ENTRY_SEARCH_H
#define DATAFLOW_ENTRY_SEARCH_H

#include "dataflow_utilities.h"

GSList* dataflow_find_subst_agents_by_kp_id(gchar* kp_id, GSList* dataflow_subst_agents);

GSList* dataflow_find_storage_entries_by_kp_id(gchar* kp_id, GSList* dataflow_inner_storage);

GSList* dataflow_find_main_agents_by_agent_id(gchar* agent_id, GSList* dataflow_main_agent_list);

GSList* dataflow_find_subst_agents_by_agent_id(gchar* agent_id, GSList* dataflow_subst_agents);

GSList* dataflow_find_main_agents_by_kp_id(gchar* kp_id, GSList* dataflow_main_agent_list);

GSList* dataflow_find_main_agents_by_substitute_agent_kp_id(gchar* kp_id,
        GSList* dataflow_main_agent_list);

GSList* dataflow_find_subst_agents_by_program_type(gchar* program_type,
        GSList* dataflow_subst_agents);

dataflow_subscr_state* dataflow_find_subscr_state_by_function_and_kp_id(DataflowFunc function,
        char* kp_id, GHashTable* dataflow_inner_subs);

dataflow_subscr_state* dataflow_find_subscr_state_by_subscr_id(char* subscr_id,
        GHashTable* dataflow_inner_subs, GMutex* dataflow_inner_subs_lock);

#endif /* DATAFLOW_ENTRY_SEARCH_H */
