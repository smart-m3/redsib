/*

  Copyright (c) 2009, Nokia Corporation
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:

    * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in
    the documentation and/or other materials provided with the
    distribution.
    * Neither the name of Nokia nor the names of its contributors
    may be used to endorse or promote products derived from this
    software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
  COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
  INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
  EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 */
#ifndef SIB_OPERATIONS_H
#define SIB_OPERATIONS_H

/*AD-ARCES
 * the LCTable implementation
 */


#include "config.h"
#include <glib.h>
#include <stdlib.h>
#include <dbus/dbus.h>
#include <sibdefs.h>

#include "LCTableTools.h"

#include <redland.h>
#include <rasqal.h>

typedef unsigned short bool;
#define true  1
#define false 0
#define TRUE  1
#define FALSE 0
#define True  1
#define False 0


#define wildcard1 	"sib:any"
#define wildcard2 	"http://www.nokia.com/NRC/M3/sib#any"

typedef ssStatus_t ss_status;

/* Definitions for enumerated SSAP protocol values */

typedef enum {M3_JOIN, M3_LEAVE, M3_INSERT, M3_REMOVE, M3_UPDATE, M3_INNER_SUBSCRIBE,
   	      M3_QUERY,M3_FIRST_SUBSCRIBE ,M3_SUBSCRIBE, M3_UNSUBSCRIBE, /*AD-ARCES*/ M3_PROTECTION_FAULT} transaction_type;

typedef enum {M3_REQUEST, M3_CONFIRMATION, M3_RESPONSE,
	      M3_INDICATION} message_type;

typedef QueryType query_type;

typedef EncodingType triple_encoding;

/* Enumeration for subscription states */

typedef enum {M3_SUB_ONGOING,  M3_SUB_PENDING, M3_SUB_STOPPED} sub_status;


typedef struct {
  ssElement_t     subject,
                  predicate,
                  object;
  ssElementType_t subjType, /* never: ssElement_TYPE_LIT*/
  /* predicate type is always uri */
                  objType;

  // Additional values
  gchar *		  subject_var;
  gchar *		  predicate_var;
  gchar *		  object_var;

  gint gp_index;
  gint indent;

} ssTriple_t_sparql;


typedef struct {
  gchar* kp_id;
  gchar* space_id;
  gint tr_id;
  transaction_type tr_type;
  message_type msg_type;
} ssap_message_header;

/* Struct for holding values from messages sent by KP */

typedef struct {
  /* Req specific fields */
  gchar* credentials;
  triple_encoding encoding;
  GSList* insert_graph;
  GSList* remove_graph;
  gchar* insert_str;
  gchar* remove_str;
  query_type type;
  gchar* query_str;
  GSList* template_query;
  ssWqlDesc_t* wql_query;
  gchar* sub_id;
} ssap_kp_message;

/* Struct for holding values from messages sent by SIB */

typedef struct {
  /* Conf / rsp / ind specific fields */
  ss_status status;
  gint ind_seqnum;
  gchar* sub_id;
  GSList* bnodes;
  gchar* bnodes_str;
  GSList* results;
  gint bool_results;

  //UNIBO
  gboolean* sparql_subscribe;
  gchar* sparql_query_str;
  gboolean* first_subscribe;

  GSList* added;
  GSList* removed;

  librdf_storage * substorage;
  librdf_model * submodel;

  gchar* results_str;
  GSList* new_results;
  gint new_bool_results;
  gchar* new_results_str;
  GSList* obsolete_results;
  gint obsolete_bool_results;
  gchar* obsolete_results_str;
} ssap_sib_message;

/* SIB common data structures */

typedef struct {
  sub_status status;
  gchar* sub_id;
  gchar* kp_id;

  /* Lock and cond needed to sync with subscribe and unsubscribe */
  GMutex* unsub_lock;
  GCond* unsub_cond;
  gboolean unsub;
} subscription_state;

typedef struct {
  /* SS name and the node (int) in piglet*/
  gchar* ss_name;
  gint ss_node;
  
  /* REDLAND enviroment */
  librdf_world* RDF_world;

  /* REDLAND Main DB */
  librdf_model* RDF_model;
  librdf_storage* RDF_storage;

  GMutex* store_lock;

  // SUBS DIFF
  //librdf_storage* RDF_storage_subscribe;
  //librdf_model* RDF_model_subscribe;

  GMutex* store_subscribe_lock;
  //end SUBs  DIFF

  /* List of joined KPs */
  GHashTable* joined;

  /* Hash table for ongoing subscriptions */
  GHashTable* subs;

  /* Variables needed to wake up scheduler when new operations arrive */
  GCond* new_reqs_cond;
  GMutex* new_reqs_lock;
  gboolean new_reqs;

  /* Locks for common data structures */
  GMutex* members_lock;
  GMutex* subscriptions_lock;


  /* Lock for waiting in init until scheduler has started */
  GMutex* scheduler_init_lock;
  GCond* scheduler_init_cond;
  gboolean scheduler_init;

  /* Operation queues for op serialization */
  GAsyncQueue* insert_queue;
  GAsyncQueue* query_queue;


  GAsyncQueue* clean_context_sub;
  /* GAsyncQueue* subscribe_queue; */


  //Communication for differential RDF subscribe
  librdf_model*   RDF_model_insert;
  librdf_storage* RDF_storage_insert;
  gboolean 	  subs_sheduler_inserted_triple;

  librdf_model*   RDF_model_remove;
  librdf_storage* RDF_storage_remove;
  gboolean 	  subs_sheduler_removed_triple;
  gboolean 	  subs_sheduler_removed_triple_with_wildcard;

  GMutex* temp_ins_rem_operations_lock;

  //sparql subscribe modules

  rasqal_world* sparql_preprocessing_world;
  GMutex* sparql_preprocessing_lock;

  //Reasoning

  GHashTable* subClasses;
  GHashTable* subProperties;
  GHashTable* propertyDomain;
  GHashTable* propertyRange;

  //PARAMETERS
  gboolean	disable_protection;
  gboolean	enable_rdf_pp;
  gboolean mem_volatile ;
  gboolean sqlite ;
  gboolean subs_persistent ;

  //DATAFLOW NETWORK DATA
  GSList* dataflow_main_agents;
  GSList* dataflow_subst_agents;
  GMutex* dataflow_operation_lock;

  GHashTable* dataflow_inner_subs;
  GMutex* dataflow_inner_subs_lock;

  GSList* dataflow_inner_storage;
  GMutex* dataflow_inner_storage_lock;
} sib_data_structure;


/* AD-ARCES
 * This modification (typedef struct SCHEDULER_ITEM{...instead:typedef struct {)
 * allow to use the scheduler item structure into the LCTaleTools and the other
 * header file that include sib_operation.h
 */

typedef struct SCHEDULER_ITEM{
  ssap_message_header* header;
  ssap_kp_message* req;
  ssap_sib_message* rsp;

  GMutex* op_lock;
  GCond* op_cond;
  gboolean op_complete;

} scheduler_item;

typedef struct {
  DBusConnection* conn;
  DBusMessage* msg;
  sib_data_structure* sib;
  transaction_type operation;
} sib_op_parameter;

typedef struct {
  /* id of joined kp*/
  gchar* kp_id;
  /* List of ongoing subscriptions for the kp */
  GSList* subs;
  gint n_of_subs;
} member_data;

sib_data_structure* sib_initialize(gchar* name, gpointer parameter_list);

/* Operation handler signatures */
/* These _must_ be started in a separate thread */

gpointer m3_join(gpointer data);

gpointer m3_leave(gpointer data);

gpointer m3_insert(gpointer data);

gpointer m3_remove(gpointer data);

gpointer m3_update(gpointer data);

gpointer m3_query(gpointer data);

gpointer m3_subscribe(gpointer data);

gpointer m3_unsubscribe(gpointer data);

#endif
