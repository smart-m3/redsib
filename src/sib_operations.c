/*

  2009, Nokia Corporation
  2012, ARCES, University of Bologna


 */
#include <glib.h>
#include <stdio.h>

#include <sib_dbus_ifaces.h>
#include <whiteboard_util.h>
#include <whiteboard_log.h>
#include <sibdefs.h>

#include <ctype.h>
#include <string.h>
#include <stdarg.h>

#include "reasoning.h"

#include <redland.h>

#include <sibmsg.h>

#include "sib_operations.h"
#include "utilities.h"


#define SIB_ROLE
#define LIBRDF_MODEL_FEATURE_CONTEXTS 		"http://feature.librdf.org/model-contexts"
#define LIBRDF_PARSER_FEATURE_ERROR_COUNT 	"http://feature.librdf.org/parser-error-count"


//RDF DEFINE
#define wildcard1 	"sib:any"
#define wildcard2 	"http://www.nokia.com/NRC/M3/sib#any"

#define fn_ex 		"http://www.w3.org/2005/xpath-functions#"
#define fn_c_1 		"fn#"
#define fn_c_2 		"fn:"

#define rdf_ex 		"http://www.w3.org/1999/02/22-rdf-syntax-ns#"
#define rdf_c 		"rdf:"

#define xsd_ex		"http://www.w3.org/2001/XMLSchema#"
#define xsd_c 		"xsd:"

#define rdfs_ex		"http://www.w3.org/2000/01/rdf-schema#"
#define rdfs_c		"rdfs:"
////////////////////////////////////////////////////////////////////



extern charStr SIB_TRIPLELIST;

extern void ssFreeTriple (ssTriple_t *triple);
extern void ssFreeTripleList (GSList **tripleList);




typedef struct {
  GAsyncQueue *insert_queue;
  GAsyncQueue *query_queue;

  GHashTable *subs;
  GMutex *subscriptions_lock;

  GCond *new_reqs_cond;
  GMutex *new_reqs_mutex;
  gboolean new_reqs;
} scheduler_param;





gpointer m3_join(gpointer data)
{
  ssap_message_header *header;
  ssap_kp_message *req_msg;
  ssap_sib_message *rsp_msg;
  //GHashTable* joined;
  //gchar* kp_id;
  gchar *credentials_rsp;
  //member_data* kp_data;

  sib_op_parameter* param = (sib_op_parameter*) data;
  /* Allocate memory for message structs */
  header =  g_new0(ssap_message_header, 1);
  req_msg = g_new0(ssap_kp_message, 1);
  rsp_msg = g_new0(ssap_sib_message, 1);


  //joined = param->sib->joined;
  /* Initialize message structs */
  header->tr_type = M3_JOIN;
  header->msg_type = M3_REQUEST;

  /* Parse message */

  if(whiteboard_util_parse_message(param->msg,
			    DBUS_TYPE_STRING, &(header->space_id),
			    DBUS_TYPE_STRING, &(header->kp_id),
			    DBUS_TYPE_INT32, &(header->tr_id),
			    DBUS_TYPE_STRING, &(req_msg->credentials),
			    DBUS_TYPE_INVALID) )
    {
	
	
      //To Better Define what a Join mean..
      /* For NOW nothing will happen except a positive feed */

      /*
      // Update the joined KPs hash table/
      kp_data = g_new0(member_data, 1);
      kp_data->kp_id = g_strdup(header->kp_id);
      kp_id = g_strdup(header->kp_id);

      kp_data->subs = NULL;
      kp_data->n_of_subs = 0;

      // LOCK MEMBER KP TABLE /
      g_mutex_lock(param->sib->members_lock);

      if (!g_hash_table_lookup(joined, kp_id))
	{
	  g_hash_table_insert(joined, kp_id, (gpointer)kp_data);
	  // UNLOCK MEMBER KP TABLE //
	  g_mutex_unlock(param->sib->members_lock);
	}
      else
	{
	  // UNLOCK MEMBER KP TABLE //
	  g_mutex_unlock(param->sib->members_lock);

	  g_free(kp_id);

	  g_free(kp_data->kp_id);
	  g_free(kp_data);

	  rsp_msg->status = ss_KPErrorRequest;
	  credentials_rsp = g_strdup("m3:KPErrorRequest");
	  goto send_response;
	}
      */

      whiteboard_log_debug("KP %s joined the smart space\n", header->kp_id);

      credentials_rsp = g_strdup("m3:Success");
      rsp_msg->status = ss_StatusOK;
    send_response:
      whiteboard_util_send_method_return(param->conn,
				  param->msg,
				  DBUS_TYPE_STRING, &(header->space_id),
				  DBUS_TYPE_STRING, &(header->kp_id),
				  DBUS_TYPE_INT32, &(header->tr_id),
				  DBUS_TYPE_INT32, &(rsp_msg->status),
				  DBUS_TYPE_STRING, &credentials_rsp,
				  WHITEBOARD_UTIL_LIST_END);
      whiteboard_log_debug("Sent response with status %d\n", rsp_msg->status);

      g_free(credentials_rsp);

      g_free(header);
      g_free(req_msg);
      g_free(rsp_msg);

    }
  else
    {
      whiteboard_log_warning("Could not parse JOIN method call message\n");
    }
  dbus_message_unref(param->msg);
  g_free(param);

  return NULL;
}

gpointer m3_leave(gpointer data)
{
  ssap_message_header *header;
  ssap_kp_message *req_msg;
  ssap_sib_message *rsp_msg;
  //GHashTable* joined;
  sib_op_parameter* param = (sib_op_parameter*) data;
  //member_data* kp_data;
  /* gchar* sub_id; */
  //GSList* i;
  /* subscription_state* sub; */


  /* Allocate memory for message structs */
  header =  g_new0(ssap_message_header, 1);
  req_msg = g_new0(ssap_kp_message, 1);
  rsp_msg = g_new0(ssap_sib_message, 1);


  //joined = param->sib->joined;
  
  /* Initialize message structs */
  header->tr_type = M3_LEAVE;
  header->msg_type = M3_REQUEST;

  /* Parse message */

  if(whiteboard_util_parse_message(param->msg,
			    DBUS_TYPE_STRING, &(header->space_id),
			    DBUS_TYPE_STRING, &(header->kp_id),
			    DBUS_TYPE_INT32, &(header->tr_id),
			    DBUS_TYPE_INVALID) )
    {

      /* For NOW nothing will happen except a positive feed */




      /* Update the joined KPs hash table*/
      /* LOCK MEMBER KP TABLE */
      /*
      g_mutex_lock(param->sib->members_lock);
      kp_data = g_hash_table_lookup(joined, header->kp_id);
      if (NULL == kp_data)
	{
	  g_mutex_unlock(param->sib->members_lock);
	  rsp_msg->status = ss_KPErrorRequest;
	  goto send_response;
	}
      */


      /* TODO: Stopping of subscriptions when leaving
      if (kp_data != NULL)
	{
	  if (kp_data->n_of_subs > 0)
	    {
	      for (i = kp_data->subs; i != NULL; i = i->next)
		{
		  sub_id = i->data;
		  g_mutex_lock(param->sib->subscriptions_lock);
		  sub = (subscription_state*)g_hash_table_lookup(param->sib->subs, req_msg->sub_id);
		  g_mutex_unlock(param->sib->subscriptions_lock);
		}

	    }
	}
      */

      /*
      g_free(kp_data->kp_id);
      for (i = kp_data->subs; i != NULL; i = i->next)
	g_free(i->data);
      g_hash_table_remove(joined, header->kp_id);
      if (kp_data)
	g_free(kp_data);
      */
      /* UNLOCK MEMBER KP TABLE */
       /*
      g_mutex_unlock(param->sib->members_lock);
      */


      rsp_msg->status = ss_StatusOK;

      whiteboard_log_debug("KP %s left the smart space\n", header->kp_id);

    send_response:
      whiteboard_util_send_method_return(param->conn,
				  param->msg,
				  DBUS_TYPE_STRING, &(header->space_id),
				  DBUS_TYPE_STRING, &(header->kp_id),
				  DBUS_TYPE_INT32, &(header->tr_id),
				  DBUS_TYPE_INT32, &(rsp_msg->status),
				  WHITEBOARD_UTIL_LIST_END);
      g_free(req_msg);
      g_free(rsp_msg);
      g_free(header);
    }

  else
    {
      whiteboard_log_warning("Could not parse LEAVE method call message\n");
    }
  dbus_message_unref(param->msg);
  g_free(param);
  return NULL;

}

gpointer m3_insert(gpointer data)
{
  ssap_message_header *header;
  ssap_kp_message *req_msg;
  ssap_sib_message *rsp_msg;
  GMutex* op_lock;
  GCond* op_cond;
  scheduler_item* s;
  ssStatus_t status = ss_StatusOK;
  sib_op_parameter* param = (sib_op_parameter*) data;

  /* Allocate memory for message structs */
  header =  g_new0(ssap_message_header, 1);
  req_msg = g_new0(ssap_kp_message, 1);
  rsp_msg = g_new0(ssap_sib_message, 1);
  s = g_new0(scheduler_item, 1);
  op_lock = g_mutex_new();
  op_cond = g_cond_new();
  rsp_msg->status = ss_StatusOK;    /* JUKKA - ARCES */

  if(whiteboard_util_parse_message(param->msg,
				   DBUS_TYPE_STRING, &(header->space_id),
				   DBUS_TYPE_STRING, &(header->kp_id),
				   DBUS_TYPE_INT32, &(header->tr_id),
				   DBUS_TYPE_INT32, &(req_msg->encoding),
				   DBUS_TYPE_STRING, &(req_msg->insert_str),
				   DBUS_TYPE_INVALID) )
    {
      whiteboard_log_debug("Parsed insert %d\n", header->tr_id);
      /* Initialize message structs */
      header->tr_type = M3_INSERT;
      header->msg_type = M3_REQUEST;
	  
      if  (EncodingRDFXML == req_msg->encoding)
        {
       	  whiteboard_log_debug("INSERT: got insert msg encoding: RDFXML\n");
	  rsp_msg->bnodes_str = g_strdup("<urilist></urilist>");
	 
        }
      /* Parse M3 triples here  */
      else if 	(EncodingM3XML == req_msg->encoding)
	{
	  status = parseM3_triples_SIB(&(req_msg->insert_graph),
				       req_msg->insert_str,
				       NULL,
				       &(rsp_msg->bnodes_str));
	  // printf("INSERT: parsed bnodes str: %s\n", rsp_msg->bnodes_str);
	  whiteboard_log_debug("Parsed M3 RDF in insert %d\n", header->tr_id);
	  if (status != ss_StatusOK)
	    {
	      
	      whiteboard_log_debug("INSERT: Parse failed, status code %d\n", status);
	      rsp_msg->bnodes_str = g_strdup("<urilist></urilist>");
	      rsp_msg->status = status;
	      goto send_response;
	    }
	}
      
      

      s->header = header;
      s->req = req_msg;
      s->rsp = rsp_msg;
      s->op_lock = op_lock;
      s->op_cond = op_cond;
      s->op_complete = FALSE;

      g_async_queue_push(param->sib->insert_queue, s);

      /* Signal scheduler that new operation has been added to queue */
      g_mutex_lock(param->sib->new_reqs_lock);
      param->sib->new_reqs = TRUE;
      g_cond_signal(param->sib->new_reqs_cond);
      g_mutex_unlock(param->sib->new_reqs_lock);

      /* Block while operation is being processed */
      g_mutex_lock(s->op_lock);
      while (!(s->op_complete))
	{
	  g_cond_wait(s->op_cond, s->op_lock);
	}
      s->op_complete = FALSE;
      g_mutex_unlock(op_lock);


    /* DAN - ARCES : XXX */
    send_response:
      /* REMOVED rsp_msg->status = status; DAN - ARCES */
      whiteboard_util_send_method_return(param->conn,
					 param->msg,
					 DBUS_TYPE_STRING, &(header->space_id),
					 DBUS_TYPE_STRING, &(header->kp_id),
					 DBUS_TYPE_INT32, &(header->tr_id),
					 DBUS_TYPE_INT32, &(rsp_msg->status),
					 DBUS_TYPE_STRING, &(rsp_msg->bnodes_str),
					 WHITEBOARD_UTIL_LIST_END);

      if (EncodingM3XML == req_msg->encoding)
	{
	  ssFreeTripleList(&(req_msg->insert_graph));
	}
      g_mutex_free(op_lock);
      g_cond_free(op_cond);
      g_free(s);
      g_free(rsp_msg->bnodes_str);
      g_free(rsp_msg);

      g_free(req_msg);
      g_free(header);

    }
  else
    {
      printf("COULD NOT PARSE INSERT DBUS MESSAGE\n");
      whiteboard_log_warning("Could not parse INSERT method call message\n");
    }
  dbus_message_unref(param->msg);
  g_free(param);
  return NULL;

}

gpointer m3_remove(gpointer data)
{
  ssap_message_header *header;
  ssap_kp_message *req_msg;
  ssap_sib_message *rsp_msg;
  GMutex* op_lock;
  GCond* op_cond;
  scheduler_item* s;
  ssStatus_t status = ss_StatusOK;
  sib_op_parameter* param = (sib_op_parameter*) data;

  /* Allocate memory for message structs */
  header =  g_new0(ssap_message_header, 1);
  req_msg = g_new0(ssap_kp_message, 1);
  rsp_msg = g_new0(ssap_sib_message, 1);
  s = g_new0(scheduler_item, 1);
  op_lock = g_mutex_new();
  op_cond = g_cond_new();
  rsp_msg->status = ss_StatusOK;    /* DAN - ARCES */

  if(whiteboard_util_parse_message(param->msg,
				   DBUS_TYPE_STRING, &(header->space_id),
				   DBUS_TYPE_STRING, &(header->kp_id),
				   DBUS_TYPE_INT32, &(header->tr_id),
				   DBUS_TYPE_INT32, &(req_msg->encoding),
				   DBUS_TYPE_STRING, &(req_msg->remove_str),
				   DBUS_TYPE_INVALID) )
    {
      whiteboard_log_debug("Parsed remove %d\n", header->tr_id);
      /* Initialize message structs */
      header->tr_type = M3_REMOVE;
      header->msg_type = M3_REQUEST;
      if (EncodingRDFXML == req_msg->encoding)
        {
       	  whiteboard_log_debug("INSERT: got insert msg encoding: RDFXML\n");
	  rsp_msg->bnodes_str = g_strdup("<urilist></urilist>");
	  
        }
      else if (EncodingM3XML == req_msg->encoding)
	{
	  status = parseM3_triples(&(req_msg->remove_graph),
				   req_msg->remove_str,
				   NULL);
	  if (status != ss_StatusOK)
	    {
	      whiteboard_log_debug("REMOVE: Parse failed, status code %d\n", status);
	      rsp_msg->status = status; /* DAN - ARCES */
	      goto send_response;
	    }
	}
      s->header = header;
      s->req = req_msg;
      s->rsp = rsp_msg;
      s->op_lock = op_lock;
      s->op_cond = op_cond;
      s->op_complete = FALSE;

      g_async_queue_push(param->sib->insert_queue, s);

      /* Signal scheduler that new operation has been added to queue */
      g_mutex_lock(param->sib->new_reqs_lock);
      param->sib->new_reqs = TRUE;
      g_cond_signal(param->sib->new_reqs_cond);
      g_mutex_unlock(param->sib->new_reqs_lock);

      /* Block while operation is being processed */
      g_mutex_lock(s->op_lock);
      while (!(s->op_complete))
	{
	  g_cond_wait(s->op_cond, s->op_lock);
	}
      s->op_complete = FALSE;
      g_mutex_unlock(op_lock);


    send_response:
      whiteboard_util_send_method_return(param->conn,
					 param->msg,
					 DBUS_TYPE_STRING, &(header->space_id),
					 DBUS_TYPE_STRING, &(header->kp_id),
					 DBUS_TYPE_INT32, &(header->tr_id),
					 DBUS_TYPE_INT32, &(rsp_msg->status),
					 WHITEBOARD_UTIL_LIST_END);

      if (EncodingM3XML == req_msg->encoding)
	{
	  ssFreeTripleList(&(req_msg->remove_graph));
	}

      g_mutex_free(op_lock);
      g_cond_free(op_cond);
      g_free(s);
      g_free(rsp_msg->bnodes_str);
      g_free(req_msg);
      g_free(rsp_msg);
      g_free(header);
    }
  else
    {
      printf("COULD NOT PARSE REMOVE DBUS MESSAGE\n");
      whiteboard_log_warning("Could not parse REMOVE method call message\n");
    }

  dbus_message_unref(param->msg);
  g_free(param);
  return NULL;
}

gpointer m3_update(gpointer data)
{
  ssap_message_header *header;
  ssap_kp_message *req_msg;
  ssap_sib_message *rsp_msg;
  GMutex* op_lock;
  GCond* op_cond;
  scheduler_item* s;
  ssStatus_t status = ss_StatusOK;
  sib_op_parameter* param = (sib_op_parameter*) data;

  /* Allocate memory for message structs */
  header =  g_new0(ssap_message_header, 1);
  req_msg = g_new0(ssap_kp_message, 1);
  rsp_msg = g_new0(ssap_sib_message, 1);
  s = g_new0(scheduler_item, 1);
  op_lock = g_mutex_new();
  op_cond = g_cond_new();
  rsp_msg->status = ss_StatusOK;    /* DAN - ARCES */

  if(whiteboard_util_parse_message(param->msg,
				   DBUS_TYPE_STRING, &(header->space_id),
				   DBUS_TYPE_STRING, &(header->kp_id),
				   DBUS_TYPE_INT32, &(header->tr_id),
				   DBUS_TYPE_INT32, &(req_msg->encoding),
				   DBUS_TYPE_STRING, &(req_msg->insert_str),
				   DBUS_TYPE_STRING, &(req_msg->remove_str),
				   DBUS_TYPE_INVALID) )
    {
      /* Initialize message structs */
      header->tr_type = M3_UPDATE;
      header->msg_type = M3_REQUEST;
      /* Parse insert and remove strings to lists of triples*/

      if (EncodingRDFXML == req_msg->encoding)
        {
       	  whiteboard_log_debug("UPDATE: got udpate msg encoding: RDFXML\n");
	  rsp_msg->bnodes_str = g_strdup("<urilist></urilist>");
	   
        }
      else if (EncodingM3XML == req_msg->encoding)
	{
	  //Remove Part
	  status = parseM3_triples(&(req_msg->remove_graph),
				   req_msg->remove_str,
				   NULL);
	  if (status != ss_StatusOK)
	    {
	      whiteboard_log_debug("UPDATE: Remove parse failed, status code %d\n", status);
	      rsp_msg->bnodes_str = g_strdup("<urilist></urilist>");
	      rsp_msg->status = status; /* DAN - ARCES */
	      goto send_response;
	    }


	  
	  status = parseM3_triples_SIB(&(req_msg->insert_graph),
				       req_msg->insert_str,
				       NULL,
				       &(rsp_msg->bnodes_str));
	  if (status != ss_StatusOK)
	    {
	      whiteboard_log_debug("UPDATE: Insert parse failed, status code %d\n", status);
	      rsp_msg->bnodes_str = g_strdup("<urilist></urilist>");
	      rsp_msg->status = status; /* DAN - ARCES */
	      goto send_response;
	    }
	}


      s->header = header;
      s->req = req_msg;
      s->rsp = rsp_msg;
      s->op_lock = op_lock;
      s->op_cond = op_cond;
      s->op_complete = FALSE;

      g_async_queue_push(param->sib->insert_queue, s);

      /* Signal scheduler that new operation has been added to queue */
      g_mutex_lock(param->sib->new_reqs_lock);
      param->sib->new_reqs = TRUE;
      g_cond_signal(param->sib->new_reqs_cond);
      g_mutex_unlock(param->sib->new_reqs_lock);

      /* Block while operation is being processed */
      g_mutex_lock(s->op_lock);
      while (!(s->op_complete))
	{
	  g_cond_wait(s->op_cond, s->op_lock);
	}
      s->op_complete = FALSE;
      g_mutex_unlock(op_lock);

  
    send_response:
    /* REMOVED rsp_msg->status = status; DAN - ARCES */
      whiteboard_util_send_method_return(param->conn,
				  param->msg,
				  DBUS_TYPE_STRING, &(header->space_id),
				  DBUS_TYPE_STRING, &(header->kp_id),
				  DBUS_TYPE_INT32, &(header->tr_id),
				  DBUS_TYPE_INT32, &(rsp_msg->status),
				  DBUS_TYPE_STRING, &(rsp_msg->bnodes_str),
				  WHITEBOARD_UTIL_LIST_END);

      if (EncodingM3XML == req_msg->encoding)
	{
	  ssFreeTripleList(&(req_msg->remove_graph));
	  ssFreeTripleList(&(req_msg->insert_graph));
	}

      g_mutex_free(op_lock);
      g_cond_free(op_cond);
      g_free(s);
      g_free(rsp_msg->bnodes_str);
      g_free(req_msg);
      g_free(rsp_msg);
      g_free(header);
    }
  else
    {
      whiteboard_log_warning("Could not parse UPDATE method call message\n");
    }

  dbus_message_unref(param->msg);
  g_free(param);
  return NULL;

}


gchar* m3_gen_triple_string(GSList* triples, sib_op_parameter* param)
{
  ssStatus_t status;
  ssBufDesc_t *bd = ssBufDesc_new();
  ssTriple_t *t;
  gchar* retval;

  status = addXML_start(bd, &SIB_TRIPLELIST, NULL, NULL, 0);
  while (status == ss_StatusOK &&
	 NULL != triples)
    {
      t = (ssTriple_t*)triples->data;
      if (NULL == t)
	{
	  status = ss_OperationFailed;
	  whiteboard_log_debug("m3_gen_triple_string(): triple was NULL\n");
	  goto end;
	}

      ////////////////////////////////////////////////////////////////
      // Expand Prefix 		//////////////////////////////////////
      //t=trasform_statement_expand(t);
      ////////////////////////////////////////////////////////////////
      

      status = addXML_templateTriple(t, NULL, (gpointer)bd);
      //printf("Added triple\n%s\n%s\n%s\n to result set\n", t->subject, t->predicate, t->object);
      triples = triples->next;
    }
 end:
  addXML_end(bd, &SIB_TRIPLELIST);
  retval = g_strdup(ssBufDesc_GetMessage(bd));
  ssBufDesc_free(&bd);
  return retval;
}


gpointer m3_query(gpointer data)
{
  ssap_message_header *header;
  ssap_kp_message *req_msg;
  ssap_sib_message *rsp_msg;
  GMutex* op_lock;
  GCond* op_cond;
  scheduler_item* s;
  ssStatus_t status;
  sib_op_parameter* param = (sib_op_parameter*) data;

   



  /* Allocate memory for message structs */
  header =  g_new0(ssap_message_header, 1);
  req_msg = g_new0(ssap_kp_message, 1);
  rsp_msg = g_new0(ssap_sib_message, 1);
  s = g_new0(scheduler_item, 1);
  op_lock = g_mutex_new();
  op_cond = g_cond_new();

  if(whiteboard_util_parse_message(param->msg,
			    DBUS_TYPE_STRING, &(header->space_id),
			    DBUS_TYPE_STRING, &(header->kp_id),
			    DBUS_TYPE_INT32, &(header->tr_id),
			    DBUS_TYPE_INT32, &(req_msg->type),
			    DBUS_TYPE_STRING, &(req_msg->query_str),
			    DBUS_TYPE_INVALID) )
    {
      /* Initialize message structs */
      header->tr_type = M3_QUERY;
      header->msg_type = M3_REQUEST;
      rsp_msg->status = ss_StatusOK;
      switch(req_msg->type)
	{
	case QueryTypeTemplate:
	  status = parseM3_triples(&(req_msg->template_query),
				   req_msg->query_str,
				   NULL);
	  break;

//WILBOUR EMULATOR :TODO?
/*
	case QueryTypeWQLValues:
	case QueryTypeWQLRelated:
	case QueryTypeWQLIsType:
	case QueryTypeWQLIsSubType:
	case QueryTypeWQLNodeTypes:
*/
//WILB EM

	case QueryTypeSPARQLSelect:
	  //printf("SPARQL!\n");
	  status = ss_StatusOK; //FOR NOW!
	  break;
	  // FALLTHROUGH //
	default: // Error //
	  printf("UNREC!");
	  rsp_msg->status = ss_SIBFailNotImpl;
	  rsp_msg->results_str = g_strdup("");
	  goto send_response;
	  break;
	}

      if (status != ss_StatusOK)
	{
	  if (status == ss_ParsingError)
	    rsp_msg->status = ss_KPErrorMsgSyntax;
	  else
	    rsp_msg->status = status;

	  rsp_msg->results_str = g_strdup("");
	  goto send_response;
	}
      s->header = header;
      s->req = req_msg;
      s->rsp = rsp_msg;
      s->op_lock = op_lock;
      s->op_cond = op_cond;
      s->op_complete = FALSE;

      g_async_queue_push(param->sib->query_queue, s);

      /* Signal scheduler that new operation has been added to queue */
      g_mutex_lock(param->sib->new_reqs_lock);
      param->sib->new_reqs = TRUE;
      g_cond_signal(param->sib->new_reqs_cond);
      g_mutex_unlock(param->sib->new_reqs_lock);

      /* Block while operation is being processed */
      g_mutex_lock(s->op_lock);
      while (!(s->op_complete))
	{
	  g_cond_wait(s->op_cond, s->op_lock);
	}
      s->op_complete = FALSE;
      g_mutex_unlock(op_lock);

      /* Generate results strings here */
      switch (req_msg->type)
	{
	case QueryTypeTemplate:
	  //g_mutex_lock(param->sib->store_lock);
	  //res_list_str = m3_triple_list_int_to_str(rsp_msg->results, param->sib->RDF_model, &status);
	  //g_mutex_unlock(param->sib->store_lock);

	  ssFreeTripleList(&(req_msg->template_query));
	  rsp_msg->results_str = m3_gen_triple_string(rsp_msg->results, param);
	  break;

//WILBOUR EMULATOR : TODO?
/*
	case QueryTypeWQLNodeTypes:
	case QueryTypeWQLValues:
	case QueryTypeWQLRelated:
	case QueryTypeWQLIsType:
	case QueryTypeWQLIsSubType:
*/
//WILB EM

	case QueryTypeSPARQLSelect:
	  //printf("SPARQL!\n");
	  req_msg->template_query=NULL;
	  //rsp_msg->results_str = m3_gen_triple_string(rsp_msg->results, param);
	  //rsp_msg->results_str = m3_gen_triple_string(NULL, param);
	  break;
	default: 
	  printf("UNREC!");
	  /* Error */
	  /* Should never be reached */
	  /* assert(0); */
	  break;
	}
    send_response:
      whiteboard_util_send_method_return(param->conn,
				  param->msg,
				  DBUS_TYPE_STRING, &(header->space_id),
				  DBUS_TYPE_STRING, &(header->kp_id),
				  DBUS_TYPE_INT32, &(header->tr_id),
				  DBUS_TYPE_INT32, &(rsp_msg->status),
				  DBUS_TYPE_STRING, &(rsp_msg->results_str),
				  WHITEBOARD_UTIL_LIST_END);
      /* Free memory*/
      switch (req_msg->type)
	{
	case QueryTypeTemplate:
	  ssFreeTripleList(&(req_msg->template_query));
	  m3_free_triple_list_simple(&(rsp_msg->results));
	  break;

//WILBOUR EMULATOR : TODO?
/*
	case QueryTypeWQLValues:
	case QueryTypeWQLRelated:
	case QueryTypeWQLIsType:
	case QueryTypeWQLIsSubType:
	case QueryTypeWQLNodeTypes:
*/
//WILB EM

	case QueryTypeSPARQLSelect:
	  //printf("SPARQL!\n");
	  break;
	  /* FALLTHROUGH */
	default:
	  printf("UNREC!");
	  /* Error */
	  /* Should not ever be reached */
	  /* assert(0); */
	  break;
	}
      g_mutex_free(op_lock);
      g_cond_free(op_cond);
      g_free(s);
      g_free(rsp_msg->results_str);
      g_free(req_msg);
      g_free(rsp_msg);
      g_free(header);
    }
  else
    {
      whiteboard_log_warning("Could not parse QUERY method call message\n");
    }

  dbus_message_unref(param->msg);
  g_free(param);
  return NULL;

}




void m3_free_triple_list_simple(GSList** triple_list_)
{
  GSList* triple_list = *triple_list_;

  ssTriple_t *t;
  while (NULL != triple_list)
    {
      t = triple_list->data;
      //printf("m3_free_triple_list: freeing triple %s %s %s\n",(char*)t->subject,(char*)t->predicate,(char*)t->object);
      triple_list = g_slist_remove(triple_list, t);
      ssFreeTriple(t);
    }

  *triple_list_ = NULL;
}



gboolean triplecompare (ssTriple_t* ta, ssTriple_t* tb)
{
	//printf("comparing: \n");
	//printf ("%s_%s_%s_%d\n", (char *)ta->subject, (char *)ta->predicate, (char *)ta->object, (bool *)ta->objType);
	//printf ("%s_%s_%s_%d\n", (char *)tb->subject, (char *)tb->predicate, (char *)tb->object, (bool *)tb->objType);
	
	if (
	   ( strcmp(ta->subject,tb->subject)==0) &&
	   ( strcmp(ta->predicate,tb->predicate)==0) &&
	   ( strcmp(ta->object,tb->object)==0) &&
	   ( ta->objType == tb->objType)
	   )
	{
		//printf("true\n\n"); 
		return TRUE;
	}

	//printf("false\n\n"); 
	return FALSE;
	
}


gboolean lists_sub_diff_triples (GSList* old_result, GSList* new_result, GSList** added_, GSList** removed_)
{
  ssTriple_t* tnew;
  ssTriple_t* told;

  GSList* new_result_tmp;
  GSList* old_result_tmp;

  new_result_tmp=new_result;
  

  // Contains m3_triple_int //
  GSList* added = *added_;
  GSList* removed = *removed_;

  gboolean* found=FALSE;

  gboolean* nothing_changed =TRUE;


  if (old_result != NULL )
  {
	 new_result_tmp=new_result;
	 for ( ; new_result_tmp != NULL ; new_result_tmp = new_result_tmp->next)
	    {
		//printf("something has been added, seeing if consistent\n\n");
		tnew = (ssTriple_t*)new_result_tmp->data;

 		old_result_tmp=old_result;
		found=FALSE;
		for ( ; old_result_tmp != NULL ; old_result_tmp = old_result_tmp->next)
		    {

			told = (ssTriple_t*)old_result_tmp->data;
			if (triplecompare(tnew, told))
			{
				found=TRUE;
				break;
			}
		    }

		if (! found)
		    {
			//printf("added in inserted, the triple is new\n\n");			
			added = g_slist_prepend(added, tnew);
			nothing_changed =FALSE;
		    }
		
	    }	
	
	 old_result_tmp=old_result;
	 for ( ; old_result_tmp != NULL ; old_result_tmp = old_result_tmp->next)
	    {
		//printf("something has been removed, seeing if consistent\n\n");
		told = (ssTriple_t*)old_result_tmp->data;

 		new_result_tmp=new_result;
		found=FALSE;
		for ( ; new_result_tmp != NULL ; new_result_tmp = new_result_tmp->next)
		    {
			tnew = (ssTriple_t*)new_result_tmp->data;
			if (triplecompare(told, tnew))
			{
				found=TRUE;
				break;
			}

		    }
		if  (! found)
		    {
			//printf("added in removed, the triple is no more present\n\n");
			removed = g_slist_prepend(removed, told);
			nothing_changed =FALSE;
		    }
	    }

  }
  else
  {
	  for ( ; new_result_tmp != NULL ; new_result_tmp = new_result_tmp->next)
	    {
		//printf("A new triple has been added\n\n");
		tnew = (ssTriple_t*)new_result_tmp->data;
		added = g_slist_prepend(added, tnew);
		nothing_changed =FALSE;
	    }


  }

  *removed_ = removed;
  *added_   = added;

  return nothing_changed;

}

void unmount_added_removed (GSList** added_, GSList** removed_)
{


  GSList* added = *added_;
  GSList* removed = *removed_;

  /*
  ssTriple_t* t;
  for ( ; added != NULL ; added = added->next)
    {
	t = (ssTriple_t*)added->data;
	added=g_slist_remove(added,t);
    }
  for ( ; removed != NULL ; removed = removed->next)
    {
	t = (ssTriple_t*)removed->data;
	removed=g_slist_remove(removed,t);
    }
  */

  g_slist_free(added);
  g_slist_free(removed);

  added=NULL;
  removed=NULL;

  *added_ =  added;
  *removed_ = removed;


}

void assign_list_to_new_pointer (GSList* source, GSList** dest_)
{
  GSList* dest = *dest_;

  ssTriple_t* t;
  for ( ; source != NULL ; source = source->next)
    {
	t = (ssTriple_t*)source->data;

	if (!t->subject || !t->predicate || !t->object)
	    {
		//If problems with wrong results..
		printf("Warning, dirty and empty triples---\n");
		source=g_slist_remove(source,t);
		ssFreeTriple(t);
	    }
	else
	    {
		dest=g_slist_prepend(dest,t);
	    }
    }

    *dest_ =  dest;
}


void  clean_context_in_sheduler(gchar* sub_id, sib_data_structure* p)
{

	librdf_statement* statement ;
	librdf_stream* stream;
	librdf_node* context_node ;

	context_node = librdf_new_node_from_uri_string(p->RDF_world, sub_id);

	stream=librdf_model_context_as_stream(p->RDF_model_subscribe, context_node);
	whiteboard_log_debug("Removing sub context \n");
	while(!librdf_stream_end(stream)) 
	{
		statement=librdf_stream_get_object(stream);
		librdf_model_context_remove_statement(p->RDF_model_subscribe, context_node, statement);
		librdf_stream_next(stream);
	}
	librdf_free_stream(stream);
	librdf_free_node(context_node);


}

gint m3_subscribe_triples(ssap_message_header *header,
			  ssap_kp_message *req_msg,
			  ssap_sib_message *rsp_msg,
			  sib_op_parameter* param)
{
  GMutex* op_lock;
  GCond* op_cond;
  scheduler_item* s;

  gchar *space_id, *kp_id;
  gchar* sub_id = rsp_msg->sub_id;
  gint tr_id;

  //GHashTable* current_result = NULL;
  GSList *added = NULL, *removed = NULL, *old_results= NULL;

  subscription_state *sub_state;

  space_id = header->space_id;
  kp_id = header->kp_id;
  tr_id = header->tr_id;

  s = g_new0(scheduler_item, 1);
  op_lock = g_mutex_new();
  op_cond = g_cond_new();

  s->header = header;
  s->req = req_msg;
  s->rsp = rsp_msg;
  s->op_lock = op_lock;
  s->op_cond = op_cond;
  s->op_complete = FALSE;
 
  gboolean * nothing_has_changed =TRUE;


  g_async_queue_push(param->sib->query_queue, s);

  /* Signal scheduler that new operation has been added to queue */
  g_mutex_lock(param->sib->new_reqs_lock);
  param->sib->new_reqs = TRUE;
  // printf("Now signaling SCHEDULER in SUBSCRIBE tr %d\n", header->tr_id);
  g_cond_signal(param->sib->new_reqs_cond);
  g_mutex_unlock(param->sib->new_reqs_lock);

  /* Block while operation is being processed */
  g_mutex_lock(s->op_lock);
  while (!(s->op_complete))
    {
      g_cond_wait(s->op_cond, s->op_lock);
    }
  s->op_complete = FALSE;
  g_mutex_unlock(op_lock);
  //printf("Got baseline query result for subscription %s\n", rsp_msg->sub_id); /* SUB_DEBUG */

  
  rsp_msg->results_str = m3_gen_triple_string(rsp_msg->results, param);

  whiteboard_util_send_method_return(param->conn,
				     param->msg,
				     DBUS_TYPE_STRING, &(space_id),
				     DBUS_TYPE_STRING, &(kp_id),
				     DBUS_TYPE_INT32, &(tr_id),
				     DBUS_TYPE_INT32, &(rsp_msg->status),
				     DBUS_TYPE_STRING, &(rsp_msg->sub_id),
				     DBUS_TYPE_STRING, &(rsp_msg->results_str),
				     WHITEBOARD_UTIL_LIST_END);

 
  g_free(rsp_msg->results_str);
  dbus_message_unref(param->msg);

  assign_list_to_new_pointer(rsp_msg->results, &old_results);
  rsp_msg->results=NULL;

  do
    {
      //printf ("Loop sub thread %s	\n", sub_id);

      /* Check if unsubscribed */
      g_mutex_lock(param->sib->subscriptions_lock);
      sub_state = (subscription_state*)g_hash_table_lookup(param->sib->subs, sub_id);
      g_mutex_unlock(param->sib->subscriptions_lock);

      if (sub_state->status == M3_SUB_STOPPED)
	  {
			g_mutex_lock(param->sib->subscriptions_lock);

			g_free(sub_state->sub_id);
			g_free(sub_state->kp_id);
			
			
			g_hash_table_remove(param->sib->subs, sub_id);
			sub_state->unsub = TRUE;

			g_cond_signal(sub_state->unsub_cond);
			/* UNLOCK SUBSCRIPTION TABLE */
			g_mutex_unlock(param->sib->subscriptions_lock);
			/* UNLOCK UNSUB LOCK */


			m3_free_triple_list_simple(&old_results);
			m3_free_triple_list_simple(&(rsp_msg->results));
			break;
	  }

      g_async_queue_push(param->sib->query_queue, s);

      g_mutex_lock(param->sib->subscriptions_lock);
      sub_state = (subscription_state*)g_hash_table_lookup(param->sib->subs, sub_id);
      g_mutex_unlock(param->sib->subscriptions_lock);

      if (sub_state->status == M3_SUB_PENDING)
	{
	  g_mutex_lock(param->sib->new_reqs_lock);
	  param->sib->new_reqs = TRUE;
	  g_cond_signal(param->sib->new_reqs_cond);
	  g_mutex_unlock(param->sib->new_reqs_lock);
	  //printf("Subscription %s pending, setting new_reqs flag\n", rsp_msg->sub_id); /* SUB_DEBUG */
	}

      /* Block while operation is processed */
      g_mutex_lock(s->op_lock);
      while (!(s->op_complete))
	  {
		g_cond_wait(s->op_cond, s->op_lock);
	  }
      s->op_complete = FALSE;
      g_mutex_unlock(op_lock);

      whiteboard_log_debug("Got new subscription result for transaction %d\n", tr_id);
      //printf("Got new query result for subscription %s\n", rsp_msg->sub_id); /* SUB_DEBUG */


      if (rsp_msg->status==ss_SubscriptionNotInvolved)
        {
	      whiteboard_log_debug("Sub not involved or unsubscribe\n");
	      //printf("Sub not involved or unsubscribe\n");
	      continue;
        }

      nothing_has_changed=lists_sub_diff_triples(old_results, rsp_msg->results, &added, &removed);

      if (nothing_has_changed)
	  {
		  whiteboard_log_debug("Nothing Changed!\n");
		  m3_free_triple_list_simple(&(rsp_msg->results));   		
		  //printf("Result for subscription %s was not changed\n", rsp_msg->sub_id); /* SUB_DEBUG */
		  whiteboard_log_debug("Subscription result was not changed for transaction %d\n", tr_id);
		  continue;
	  }


      rsp_msg->new_results_str = m3_gen_triple_string(added, param);
      rsp_msg->obsolete_results_str = m3_gen_triple_string(removed, param);


      if( ++(rsp_msg->ind_seqnum) == SSAP_IND_WRAP_NUM )
		rsp_msg->ind_seqnum=1;

      whiteboard_util_send_signal(SIB_DBUS_OBJECT,
				  SIB_DBUS_KP_INTERFACE,
				  SIB_DBUS_KP_SIGNAL_SUBSCRIPTION_IND,
				  param->conn,
				  DBUS_TYPE_STRING, &(header->space_id),
				  DBUS_TYPE_STRING, &(header->kp_id),
				  DBUS_TYPE_INT32, &(header->tr_id),
				  DBUS_TYPE_INT32, &(rsp_msg->ind_seqnum),
				  DBUS_TYPE_STRING, &(rsp_msg->sub_id),
				  DBUS_TYPE_STRING, &(rsp_msg->new_results_str),
				  DBUS_TYPE_STRING, &(rsp_msg->obsolete_results_str),
				  WHITEBOARD_UTIL_LIST_END);

      //printf("Sent new result for sub %s, seqnum %d to transport\n",	     rsp_msg->sub_id, rsp_msg->ind_seqnum); /* SUB_DEBUG */


       /* Free memory 2 */

      m3_free_triple_list_simple(&old_results);
      unmount_added_removed (&added, &removed);
 
      assign_list_to_new_pointer(rsp_msg->results, &old_results);
      rsp_msg->results=NULL;


      whiteboard_log_debug("Subscription result for transaction %d is\n%s\n%s\n", tr_id,
			   rsp_msg->new_results_str,
			   rsp_msg->obsolete_results_str);

      g_free(rsp_msg->new_results_str);
      g_free(rsp_msg->obsolete_results_str);
    }
  while(TRUE);

  g_cond_free(op_cond);
  g_mutex_free(op_lock);
  g_free(s);
  return ss_StatusOK;
}


// This only will work with drammatic heavy persistent queries..
// Commented is okay from our point of view ;)  (Arces)
/* 
gint m3_subscribe_SPARQL(ssap_message_header *header,ssap_kp_message *req_msg, ssap_sib_message *rsp_msg, sib_op_parameter* param){}
*/

gpointer m3_subscribe(gpointer data)
{
  ssap_message_header *header;
  ssap_kp_message *req_msg;
  ssap_sib_message *rsp_msg;

  ssStatus_t status;
  sib_op_parameter* param = (sib_op_parameter*) data;

  gchar *space_id, *kp_id;
  gchar *temp_sub_id = NULL;
  gint tr_id;
  //member_data* kp_data;
  subscription_state* sub_state;

  /* Allocate memory for message structs */
  header =  g_new0(ssap_message_header, 1);
  req_msg = g_new0(ssap_kp_message, 1);
  rsp_msg = g_new0(ssap_sib_message, 1);

  /* Initialize message structs */
  header->tr_type = M3_SUBSCRIBE;
  header->msg_type = M3_REQUEST;

  sub_state = g_new0(subscription_state, 1);

  if(whiteboard_util_parse_message(param->msg,
			    DBUS_TYPE_STRING, &space_id,
			    DBUS_TYPE_STRING, &kp_id,
			    DBUS_TYPE_INT32, &tr_id,
			    DBUS_TYPE_INT32, &(req_msg->type),
			    DBUS_TYPE_STRING, &(req_msg->query_str),
			    DBUS_TYPE_INVALID) )
    {
      header->space_id = g_strdup(space_id);
      header->kp_id = g_strdup(kp_id);
      header->tr_id = tr_id;

      /* ASSIGN SUB ID HERE */
      temp_sub_id = g_strdup_printf("%s_%d", kp_id, tr_id);

      g_mutex_lock(param->sib->subscriptions_lock);
      while (g_hash_table_lookup(param->sib->subs, temp_sub_id)) {
	/* Create better sub_id generator! */
	g_free(temp_sub_id);
	temp_sub_id = g_strdup_printf("%s_%d", kp_id, ++tr_id);
      }

      sub_state->status = M3_SUB_ONGOING;
      sub_state->sub_id = g_strdup(temp_sub_id);
      sub_state->kp_id = g_strdup(kp_id);

      g_hash_table_insert(param->sib->subs, temp_sub_id, sub_state);
      g_mutex_unlock(param->sib->subscriptions_lock);


      // TO DEFINE.. Commented because now just a waste of memory

      //g_mutex_lock(param->sib->members_lock);
      //kp_data = g_hash_table_lookup(param->sib->joined, kp_id);
      //if (kp_data != NULL)
	//{
	//  kp_data->subs = g_slist_prepend(kp_data->subs, g_strdup(temp_sub_id));
	//  kp_data->n_of_subs++;
	//}
      //g_mutex_unlock(param->sib->members_lock);

      rsp_msg->sub_id = g_strdup(temp_sub_id);
      rsp_msg->first_subscribe =TRUE;


      /* Initialize the query structure and start
	 suitable subscription processor */
      switch(req_msg->type)
	{
		case QueryTypeTemplate:
		  status = parseM3_triples(&(req_msg->template_query),
					   req_msg->query_str,
					   NULL);
		  if (status == ss_StatusOK)
		    {
		      //printf("Started triples subscription with id %s", rsp_msg->sub_id); /* SUB_DEBUG */
		      status = m3_subscribe_triples(header, req_msg, rsp_msg, param);
		    }
		  break;
	 
	}
      //printf("SUBSCRIBE: subscription %s finished \n", rsp_msg->sub_id);
    }
  else
    {
      whiteboard_log_warning("Could not parse SUBSCRIBE method call message\n");
    }



  g_free(temp_sub_id);
  
  m3_free_triple_list_simple(&(req_msg->template_query));
  g_free(req_msg);

  g_free(rsp_msg->sub_id);
  g_free(rsp_msg);

  g_free(header->space_id);
  g_free(header->kp_id);
  g_free(header);


  g_free(param);
  return NULL;

}

gpointer m3_unsubscribe(gpointer data)
{
  ssap_message_header *header;
  ssap_kp_message *req_msg;
  ssap_sib_message *rsp_msg;
  sib_op_parameter* param = (sib_op_parameter*) data;

  GCond* unsub_cond;
  subscription_state* sub;

  gchar* sub_id;


  unsub_cond = g_cond_new();
  /* Allocate memory for message structs */
  header =  g_new0(ssap_message_header, 1);
  req_msg = g_new0(ssap_kp_message, 1);
  rsp_msg = g_new0(ssap_sib_message, 1);

  if(whiteboard_util_parse_message(param->msg,
			    DBUS_TYPE_STRING, &(header->space_id),
			    DBUS_TYPE_STRING, &(header->kp_id),
			    DBUS_TYPE_INT32, &(header->tr_id),
			    DBUS_TYPE_STRING, &(req_msg->sub_id),
			    DBUS_TYPE_INVALID) )
    {
	g_mutex_lock(param->sib->subscriptions_lock);
	sub = (subscription_state*)g_hash_table_lookup(param->sib->subs, req_msg->sub_id);
	g_mutex_unlock(param->sib->subscriptions_lock);

    	if (NULL != sub)
	  {
		  whiteboard_log_debug("UNSUBSCRIBE: Found sub for sub id %s\n", req_msg->sub_id);

		  /* Set subscription status to stopped */
		  g_mutex_lock(param->sib->subscriptions_lock);

		  sub->unsub_cond = unsub_cond;
		  sub->unsub = FALSE;
		  sub->status = M3_SUB_STOPPED;
		  g_mutex_unlock(param->sib->subscriptions_lock);

		  /* Signal scheduler to execute a round to get
		     all subscriptions to check their status
		     TODO: better way to signal subscriptions
		  */
		  g_mutex_lock(param->sib->new_reqs_lock);
		  param->sib->new_reqs = TRUE;
		  g_cond_signal(param->sib->new_reqs_cond);
		  g_mutex_unlock(param->sib->new_reqs_lock);

		  /* Wait until subscription processing has finished */

		  g_mutex_lock(param->sib->subscriptions_lock);
		  while(!sub->unsub)
		    {
		      g_cond_wait(unsub_cond, param->sib->subscriptions_lock);
		    }
		  sub->unsub = FALSE;

		  g_free(sub);

		  g_mutex_unlock(param->sib->subscriptions_lock);

		  rsp_msg->status = ss_StatusOK;
	  }
	else
	  rsp_msg->status = ss_KPErrorRequest;

	whiteboard_util_send_method_return(param->conn,
					 param->msg,
					 DBUS_TYPE_STRING, &(header->space_id),
					 DBUS_TYPE_STRING, &(header->kp_id),
					 DBUS_TYPE_INT32, &(header->tr_id),
					 DBUS_TYPE_INT32, &(rsp_msg->status),
					 DBUS_TYPE_STRING, &(req_msg->sub_id),
					 WHITEBOARD_UTIL_LIST_END);

	whiteboard_log_debug("UNSUBSCRIBE: Sent unsub cnf for sub id %s\n", req_msg->sub_id);

	///////////Add to queue Context/////
	sub_id=g_strdup(req_msg->sub_id);
	g_async_queue_push(param->sib->clean_context_sub, sub_id);
	/////////////////////////////

    }
  else
    {
      whiteboard_log_warning("Could not parse UNSUBSCRIBE method call message\n");
    }



  g_free(req_msg);
  g_free(rsp_msg);

  g_free(header);

  g_cond_free(unsub_cond);

  dbus_message_unref(param->msg);
  g_free(param);

  return NULL;
}


ssStatus_t rdf_writer(scheduler_item* op, sib_data_structure* param)
{
  
  switch (op->req->encoding)
    {
    case EncodingM3XML:
      whiteboard_log_debug("Writing RDF/M3 in transaction %d\n", op->header->tr_id);
      GSList* i ;
      ssTriple_t* t;


       

      //Rdf Redland Initialize..
      librdf_statement* statement ;


      ///////////////////////////

  
      for (i = op->req->insert_graph;
	   i != NULL;
	   i = i->next)
	{
	  t = (ssTriple_t*)i->data;

	      	  
	  /*	DEBUG PRINT
	  fprintf(stderr,"Ins : %s, p: %s, o: %s, o_ty: %d",
			       (unsigned char*)t->subject,
			       (unsigned char*)t->predicate,
			       (unsigned char*)t->object,
			       (bool*)(ssElementType_t)t->objType);
	  // disable
	  */ 		


	  //Reasoning, prefix transform 
	  //t=trasform_statement_contract(t);
	  t=trasform_statement_expand(t);
	  //////////////////////////////////////////

	  //Insert statement
	  statement=librdf_new_statement(param->RDF_world);

	  librdf_statement_set_subject(statement, 
		                       librdf_new_node_from_uri_string(param->RDF_world,  t->subject));
	  
	  librdf_statement_set_predicate(statement,
		                       librdf_new_node_from_uri_string(param->RDF_world, t->predicate));

	  if (! t->objType) {	
		  //URI: 	t->objType==0
		  librdf_statement_set_object(statement, 
					librdf_new_node_from_uri_string(param->RDF_world,  t->object));
	  }
	  else
	  {	
		  //Literal:  	t->objType==1
		  librdf_statement_set_object(statement,
                              		librdf_new_node_from_literal(param->RDF_world, t->object, NULL, 0));					

	  }
	  librdf_model_add_statement(param->RDF_model, statement);


	  //FOR DIFFERENTIAL SUBSCRIBE
	  //g_mutex_lock(param->temp_ins_rem_operations_lock);
	  librdf_model_add_statement(param->RDF_model_insert, statement);
	  ///////////////////////////////////////Reasoning!
	  reasoning(param,t, param->enable_rdf_pp);
	  ////////////////////////////////////////////////
	  param->subs_sheduler_inserted_triple=TRUE;
	  //g_mutex_unlock(param->temp_ins_rem_operations_lock);


	  librdf_free_statement(statement);



	}

      op->rsp->status = ss_StatusOK;
      break;
   
    case EncodingRDFXML:
      {

	      librdf_parser* parser;
	      librdf_uri* uri;
	      librdf_stream* stream;
	      librdf_statement* statement;
	     
  	      gboolean * blanks_founded;

	      uri=librdf_new_uri(param->RDF_world, "http://example.librdf.org/");  
	      parser=librdf_new_parser(param->RDF_world, "rdfxml", NULL, NULL);

	      stream=librdf_parser_parse_string_as_stream (parser, op->req->insert_str,uri);

	      if (!stream)
	      {
		fprintf(stderr, "Failed to parse RDF into model\n");
		op->rsp->status = ss_OperationFailed;
		// No bnodes to uri mapping from RDF/XML content 
		// op->rsp->bnodes_str = g_strdup("<urilist></urilist>");
	      }
	      else
	      {
		//FOR DIFFERENTIAL SUBSCRIBE
		//g_mutex_lock(param->temp_ins_rem_operations_lock);
	  	param->subs_sheduler_inserted_triple=TRUE;
	  	/////////////////////////////

	 	while(!librdf_stream_end(stream))
		  {
			statement= librdf_stream_get_object(stream);

			librdf_model_add_statement(param->RDF_model, statement);
			librdf_model_add_statement(param->RDF_model_insert, statement);

			blanks_founded=FALSE;

			ssTriple_t *ttemp = (ssTriple_t *)g_new0(ssTriple_t,1);
			g_return_val_if_fail(ttemp, ss_NotEnoughResources);

			if ( ! librdf_node_is_blank(librdf_statement_get_subject(statement)))
			{
				ttemp->subject=	   g_strdup(librdf_uri_as_string(librdf_node_get_uri(librdf_statement_get_subject(statement))));
			}
			else //Bnode
			{
				blanks_founded=TRUE;
			}

			ttemp->predicate = g_strdup(librdf_uri_as_string(librdf_node_get_uri(librdf_statement_get_predicate(statement))));

			if (librdf_node_is_literal(librdf_statement_get_object(statement)))
			{	

				ttemp->object =  g_strdup(librdf_node_get_literal_value(librdf_statement_get_object(statement)));
				ttemp->objType = ssElement_TYPE_LIT; 
			}
			else if ( ! librdf_node_is_blank(librdf_statement_get_object(statement)))
			{	
				
				ttemp->object =  g_strdup(librdf_uri_as_string( librdf_node_get_uri(librdf_statement_get_object(statement))));
				ttemp->objType = ssElement_TYPE_URI; 
			}
			else  //Bnode
			{
				blanks_founded=TRUE;
			}			

			/////////////////////Reasoning!
			if (! blanks_founded)
			{
				reasoning(param,ttemp, param->enable_rdf_pp);
			}
			//////////////////////////////

			ssFreeTriple(ttemp);

			librdf_stream_next(stream);
		  }
		//g_mutex_unlock(param->temp_ins_rem_operations_lock);
	

		//fprintf(stderr, "RDF inserted ..\n");
		op->rsp->status = ss_StatusOK;
	      }
	      

	      librdf_free_statement(statement);
	      librdf_free_stream(stream);
	      librdf_free_parser(parser); 
	      librdf_free_uri(uri); 
	    
	      break;

      }
    default: /* ERROR CASE */
      op->rsp->status = ss_InvalidParameter;
      //op->rsp->bnodes_str = g_strdup("<urilist></urilist>");
      break;
    }
  return op->rsp->status;
}

ssStatus_t rdf_retractor(scheduler_item* op, sib_data_structure* param)
{

  switch (op->req->encoding)
    {
    case EncodingM3XML:
	{
	  whiteboard_log_debug("Removing in transaction %d\n", op->header->tr_id);

	  GSList *i;
	  ssTriple_t* tq;
	  

	  //Rdf Redland Initialize..
	  librdf_statement* 	statement ;
	  librdf_statement*	partial_statement;
	  librdf_stream* 	stream;
	  librdf_statement* 	remove_statement_for_subs;
	  ///////////////////

	  gboolean* al_least_one_wildcard=0;
	  gboolean* all_wildcard=0;
	  

	  
	  for (i = op->req->remove_graph; i != NULL; i = i->next)
	  {
	      	  tq = (ssTriple_t*)i->data;


		  //////Reasoning, prefix transform 
		  //tq=trasform_statement_contract(tq);
		  tq=trasform_statement_expand(tq);
		  //////////////////////////////////


		  al_least_one_wildcard=0;
		  all_wildcard=1;

		  partial_statement=librdf_new_statement(param->RDF_world);

		  if ( (strcmp(tq->subject,wildcard1) ==0) || ( (strcmp(tq->subject,wildcard2) ==0) ))
		    {	librdf_statement_set_subject(partial_statement, NULL); 
			//fprintf (stderr,"wildcard subj enable ");
			al_least_one_wildcard=1;
		    }
		  else 
		    {	librdf_statement_set_subject(partial_statement, librdf_new_node_from_uri_string(param->RDF_world,  tq->subject));	
			all_wildcard=0;
			//fprintf (stderr,"no wildcard subj enable");
		    }

		  if ( (strcmp(tq->predicate,wildcard1) ==0) || ( (strcmp(tq->predicate,wildcard2) ==0) ))
		    {	librdf_statement_set_predicate(partial_statement, NULL);
			//fprintf (stderr,va"wildcard pred enable ");
			al_least_one_wildcard=1;
		    }
		  else 
		    {	librdf_statement_set_predicate(partial_statement, librdf_new_node_from_uri_string(param->RDF_world,  tq->predicate));	
			all_wildcard=0;
		    }

		  if ( (strcmp(tq->object,wildcard1) ==0) || ( (strcmp(tq->object,wildcard2) ==0) ))
		    {	librdf_statement_set_object(partial_statement, NULL); 
			//fprintf (stderr,"wildcard obj enable ");
			al_least_one_wildcard=1;
		    }
		  else 
		    {	
			  all_wildcard=0;
			  if (! tq->objType) {	
			
				  librdf_statement_set_object(partial_statement, 
							librdf_new_node_from_uri_string(param->RDF_world,  tq->object));
			  }
			  else
			  {						
				  librdf_statement_set_object(partial_statement,
				              		librdf_new_node_from_literal(param->RDF_world,tq->object, NULL, 0));
			  }
		    }

		  ////////////////////////////////////////////////////////
		  //FOR DIFFERENTIAL SUBSCRIBE
		  //REMOVE FOR SUBS

		  whiteboard_log_debug("start insert on remove ts\n");
		  //g_mutex_lock(param->temp_ins_rem_operations_lock);
		  
		  remove_statement_for_subs=librdf_new_statement(param->RDF_world);

		  librdf_statement_set_subject(remove_statement_for_subs, librdf_new_node_from_uri_string(param->RDF_world, tq->subject));
		  librdf_statement_set_predicate(remove_statement_for_subs, librdf_new_node_from_uri_string(param->RDF_world,tq->predicate));
		  if (! tq->objType) 
			{	
				  librdf_statement_set_object(remove_statement_for_subs, 
							librdf_new_node_from_uri_string(param->RDF_world,tq->object));
			}
			else
			{						
				  librdf_statement_set_object(remove_statement_for_subs,
				              		librdf_new_node_from_literal(param->RDF_world, tq->object, NULL, 0));
			}

		  librdf_model_add_statement(param->RDF_model_remove, remove_statement_for_subs);
		  librdf_free_statement(remove_statement_for_subs);


		  param->subs_sheduler_removed_triple=TRUE;
		  param->subs_sheduler_removed_triple_with_wildcard = al_least_one_wildcard;

		  //g_mutex_unlock(param->temp_ins_rem_operations_lock);
		  whiteboard_log_debug("end insert on remove ts\n");
		  //////////////////////////////////////////////////////////


		  if (al_least_one_wildcard)
		  {
			  // TODO
			  //if (all_wildcard)
			  //{
				  //If the TS is big I can't copy all the content in ram.. so I need this solution.
				  //printf("ALL WC!\n");
				  //librdf_free_model(param->RDF_model);
				  //librdf_free_storage(param->RDF_storage);
				  //param->RDF_model=librdf_new_model(param->RDF_world, param->RDF_storage , NULL);
			  //}
			  //else
			  //{
				  stream=librdf_model_find_statements(param->RDF_model, partial_statement);

			 	  while(!librdf_stream_end(stream))
				  {
					statement=librdf_stream_get_object(stream);
					librdf_model_remove_statement(param->RDF_model, statement);
					librdf_stream_next(stream);
				  }
				  librdf_free_stream(stream);
			  //}
		  }
		  else
	 	  {
			  librdf_model_remove_statement(param->RDF_model, partial_statement);

		  }
		  librdf_free_statement(partial_statement);

	  }
	  
	  op->rsp->status = ss_StatusOK;
	  return op->rsp->status;



        }
    case EncodingRDFXML:
        {
	      librdf_parser* parser;
	      librdf_uri* uri;
	      librdf_stream* stream;
	      librdf_statement* statement ;


	      uri=librdf_new_uri(param->RDF_world, "http://example.librdf.org/");  
	      parser=librdf_new_parser(param->RDF_world, "rdfxml", NULL, NULL);

	      stream=librdf_parser_parse_string_as_stream (parser, op->req->remove_str,uri);

	      if (!stream)
	      {
				fprintf(stderr, "Failed to parse RDF into model\n");
				op->rsp->status = ss_OperationFailed;
				// No bnodes to uri mapping from RDF/XML content
				// op->rsp->bnodes_str = g_strdup("<urilist></urilist>");
	      }
	      else
	      {

				//g_mutex_lock(param->temp_ins_rem_operations_lock);
				param->subs_sheduler_inserted_triple=TRUE;

				while(!librdf_stream_end(stream))
				  {
					statement= librdf_stream_get_object(stream);
					librdf_model_remove_statement(param->RDF_model, statement);
					librdf_model_add_statement(param->RDF_model_remove, statement);
					librdf_stream_next(stream);
				  }
				//g_mutex_unlock(param->temp_ins_rem_operations_lock);

		
				  fprintf(stderr, "RDF removed ..\n");
				  op->rsp->status = ss_StatusOK;
	      }
	      

	      librdf_free_statement(statement);
	      librdf_free_stream(stream);
	      librdf_free_parser(parser); 
	      librdf_free_uri(uri); 

	      return op->rsp->status;
        }
    }
}




ssStatus_t rdf_reader(scheduler_item* op, sib_data_structure* p)
{
  whiteboard_log_debug("Querying in transaction %d\n", op->header->tr_id);


  gchar*	XML_no_pre;
  gchar*	XML_no_pre_nospace;
  gchar*	result_sparql_query;
  size_t 	length;

  //Rdf Redland Initialize..
  librdf_statement* 	statement ;
  librdf_statement*	partial_statement;
  librdf_stream* stream;
  librdf_stream* stream2; 

  librdf_node* sub_node ;
  librdf_node* pred_node;
  librdf_node* obj_node ;
  librdf_node* context_node ;
  /////////////////////////


  if ((op->header->tr_type == M3_QUERY) || (op->rsp->first_subscribe))   //It could be first SUBSCRIBE or a normal rdf query
    {

	  if (op->rsp->first_subscribe)
	  {
		  //Initialize context Node with subscription name
		  context_node = librdf_new_node_from_uri_string(p->RDF_world, op->rsp->sub_id);
		  whiteboard_log_debug("QUERY FOR FIRST SUBS: %s to ongoing\n",op->rsp->sub_id); 
	  }

	  whiteboard_log_debug("Query Executing (or first subscribe):\n");
	  switch (op->req->type)
	    {
	    case QueryTypeTemplate:
	      {
		ssTriple_t* tq;
		GSList* query_list = op->req->template_query;
		 
		 
		ssTriple_t *triple;

		//Clean old Results
      		//ssFreeTripleList(op->rsp->results);
		op->rsp->results = NULL;

		//whiteboard_log_debug("Doing template query");
		/* Changed if anything goes wrong */
		op->rsp->status = ss_StatusOK;

		/* Iterate over query here */
		while (query_list != NULL) {


		  tq = query_list->data;
		  if (!tq->subject || !tq->predicate || !tq->object)
		    {
		      op->rsp->status = ss_OperationFailed;
		      op->rsp->results = NULL;
		      break;
		    }


		  //Reasoning, prefix transform 
		  //tq=trasform_statement_contract(tq);
		  tq=trasform_statement_expand(tq);
		  //////////////////////////////////


		  //printf("RDF reader: now querying for transaction %d\n", op->header->tr_id);
		  //printf("RDF reader: triple is \n%s\n%s\n%s\n", tq->subject, tq->predicate, tq->object);


		  // Redland query with wildcard support
		  partial_statement=librdf_new_statement(p->RDF_world);

		  if ( (strcmp(tq->subject,wildcard1) ==0) || ( (strcmp(tq->subject,wildcard2) ==0) ))
		    {	librdf_statement_set_subject(partial_statement, NULL); 
		    }
		  else 
		    {	librdf_statement_set_subject(partial_statement, librdf_new_node_from_uri_string(p->RDF_world,  tq->subject));	
		    }

		  if ( (strcmp(tq->predicate,wildcard1) ==0) || ( (strcmp(tq->predicate,wildcard2) ==0) ))
		    {	librdf_statement_set_predicate(partial_statement, NULL);
		    }
		  else 
		    {	librdf_statement_set_predicate(partial_statement, librdf_new_node_from_uri_string(p->RDF_world,tq->predicate));	
		    }

		  if ( (strcmp(tq->object,wildcard1) ==0) || ( (strcmp(tq->object,wildcard2) ==0) ))
		    {	librdf_statement_set_object(partial_statement, NULL); 
		    }
		  else 
		    {	
			  if (! tq->objType) {	
			
				  librdf_statement_set_object(partial_statement, 
							librdf_new_node_from_uri_string(p->RDF_world, tq->object));
			  }
			  else
			  {						
				  librdf_statement_set_object(partial_statement,
				              		librdf_new_node_from_literal(p->RDF_world, tq->object, NULL, 0));
			  }
		    }

		  stream=librdf_model_find_statements(p->RDF_model, partial_statement);
		  librdf_free_statement(partial_statement);



		  //fprintf(stderr, "Query print all\n");
	 	  while(!librdf_stream_end(stream)) {
			statement=librdf_stream_get_object(stream);

			

			//CHARGE FIRST TIME LOCAL TRIPLES IN LOCAL CONTEXT STORE///////////////////////////////////
			if (op->rsp->first_subscribe)
			{	
				librdf_model_context_add_statement(p->RDF_model_subscribe,context_node, statement);
				
			}
			///////////////////////////////////////////////////////////////////////////////////////////


			sub_node  = librdf_statement_get_subject(statement);
			pred_node = librdf_statement_get_predicate(statement);		
			obj_node  = librdf_statement_get_object(statement);

			if (( librdf_node_is_blank(sub_node))||( librdf_node_is_blank(obj_node)))
			{
				librdf_stream_next(stream);
				continue;
			}


			triple  =(ssTriple_t *) g_new0(ssTriple_t,1);
			g_return_val_if_fail(triple, ss_NotEnoughResources);


			triple->subject=	g_strdup(librdf_uri_as_string( librdf_node_get_uri(sub_node)));
			triple->predicate =	g_strdup(librdf_uri_as_string( librdf_node_get_uri(pred_node)));
		

			if (librdf_node_is_literal(obj_node))
			{	
				triple->object =  g_strdup(librdf_node_get_literal_value(obj_node));
				triple->objType = ssElement_TYPE_LIT; 
			}
			else
			{	
				triple->object =  g_strdup(librdf_uri_as_string( librdf_node_get_uri(obj_node)));
				triple->objType = ssElement_TYPE_URI; 
			}



			op->rsp->results = g_slist_prepend(op->rsp->results, triple);
			
			librdf_stream_next(stream);
		  }
		  
		  librdf_free_stream(stream);
		  // -redland
		  query_list = query_list->next;
		}
	      break;
	      }

	 // WILBOUR QUERY EMULATOR      TODO?
         /*
	    case QueryTypeWQLValues:
	      {	      }
	    case QueryTypeWQLNodeTypes:
	      {	      }
	    case QueryTypeWQLRelated:
	      {	      }
	    case QueryTypeWQLIsType:
	      {	      }
	    case QueryTypeWQLIsSubType:
	      {	      }
	 /*  EM WQL TODO */


	    case QueryTypeSPARQLSelect:
	    {
		//Inizialize specific to SPARQL
		librdf_query_results* results;
		librdf_query* 	query;
		gchar*		prefix_query_str;
		gchar*		prefix_str;

		//printf("RECEIVED SPARQL IN\n");
		//printf("q: %s\n",op->req->query_str);

		prefix_str= g_strdup_printf("PREFIX %s <%s> PREFIX %s <%s> PREFIX %s <%s> PREFIX %s <%s>  ",rdf_c ,rdf_ex ,fn_c_2,fn_ex,rdfs_c,rdfs_ex,xsd_c , xsd_ex);

		prefix_query_str=g_strdup_printf("%s %s",prefix_str,op->req->query_str);
		query=librdf_new_query(p->RDF_world, "sparql", NULL,prefix_query_str,NULL);
	
		//printf("%s\n",prefix_query_str);
		g_free(prefix_str);
		g_free(prefix_query_str);

		results=librdf_model_query_execute(p->RDF_model, query);

		if (results ==NULL)
		{
			whiteboard_log_debug("SPARQL syntax error! \n");
			librdf_free_query_results(results);
			librdf_free_query(query);
			op->rsp->results_str=g_strdup("");
			op->rsp->status = ss_KPErrorMsgSyntax;
		}
		else
		{	
			
			if (librdf_query_results_is_graph(results))
			{
				//RESULT GRAPH CASE
				librdf_serializer* serializer;
				serializer = librdf_new_serializer(p->RDF_world, NULL, NULL, NULL);
				stream2=librdf_query_results_as_stream(results); 
				result_sparql_query = librdf_serializer_serialize_stream_to_counted_string(serializer,NULL, stream2, &length);
				librdf_free_stream(stream2);
				XML_no_pre= redland_XML_no_preamble(result_sparql_query);    
				//XML_no_pre_nospace=space_deleter_construct(XML_no_pre); //not now..
				op->rsp->results_str=XML_no_pre; 

			}
			else
			{
				//RESULT XML CASE
				result_sparql_query= librdf_query_results_to_string(results ,NULL,NULL);

				XML_no_pre= redland_XML_no_preamble(result_sparql_query);
				XML_no_pre_nospace=space_deleter(XML_no_pre);
				op->rsp->results_str=XML_no_pre_nospace; 
			}

			
			librdf_free_query_results(results);
			librdf_free_query(query);
			op->rsp->status = ss_StatusOK;
		}
		break;
	    }
        //End case query type
	}


	if (op->rsp->first_subscribe) //First subscribe ending.
	{
		  op->rsp->first_subscribe=FALSE;
		  librdf_free_node(context_node);


		  subscription_state* s;
		  g_mutex_lock(p->subscriptions_lock);
		  s = g_hash_table_lookup(p->subs, op->rsp->sub_id);
		  if (NULL != s && s->status != M3_SUB_STOPPED)
		  {
			  s->status = M3_SUB_ONGOING;
			  whiteboard_log_debug("QUERY FOR FIRST SUBS: Set subscription %s to ongoing\n", s->sub_id); /* SUB_DEBUG */
		  }
		  g_mutex_unlock(p->subscriptions_lock);


	}

	/////////////////////////////
	op->op_complete = TRUE;
        g_cond_signal(op->op_cond);
        /////////////////////////////
        
        op->rsp->status = ss_StatusOK;
        return op->rsp->status;
    }
    else
    {

      // I got a SUBS query, it's query on current sheduler cycle of insert / remove.
      // No other persistent db access will be required.
      persistent_query_emulator( op,  p);
      ///////////////////////////////////////////////////////////////////////////////
      
      return op->rsp->status;

    } //End case subscribe or query&first sub

}


//Persistent query emulator. 
//It emulate persistent query checking sheduled insert/remove operations in every sheduled cycle
void persistent_query_emulator(scheduler_item* op, sib_data_structure* p)
{
	//char* str_tmp_exp;
	gchar *chomped_s;
	gchar *chomped_p;
	gchar *chomped_o;


	//Rdf Redland Initialize..
	librdf_statement* 	statement ;
	librdf_statement*	partial_statement;
	librdf_stream* stream;

	librdf_node* sub_node ;
	librdf_node* pred_node;
	librdf_node* obj_node ;
	librdf_node* context_node ;

	librdf_statement*	statement_nested ;
	librdf_statement*	partial_statement_nested ;
	librdf_stream* stream_nested;
	//////////////////////////////////////////////


	gboolean really_do_query_on_cache=0;

	gboolean wildcard_on_remove_s=0;
	gboolean wildcard_on_remove_p=0;
	gboolean wildcard_on_remove_o=0;		

	gboolean same_s=0;
	gboolean same_p=0;
	gboolean same_o=0;	

	gboolean still_present=0;


	/////// Default, If nothing has changed //////
	op->rsp->status =  ss_SubscriptionNotInvolved;
	op->rsp->results = NULL;
	//////////////////////////////////////////////

	//Check if something has been added or removed..
	if (p->subs_sheduler_inserted_triple || p->subs_sheduler_removed_triple)
	{
	  
	  switch (op->req->type)
	    {
	    case QueryTypeTemplate:
	      {
		ssTriple_t* tq;
		GSList* query_list = op->req->template_query;


		//Set a new context 
		context_node = librdf_new_node_from_uri_string(p->RDF_world, op->rsp->sub_id);

		/* Iterate over query here */
		while (query_list != NULL) 
		{

		  tq = (ssTriple_t*)query_list->data;
		  if (!tq->subject || !tq->predicate || !tq->object)
		  {
		      op->rsp->status = ss_OperationFailed;
		      op->rsp->results = NULL;
		      break;
		  }
	

		  partial_statement=librdf_new_statement(p->RDF_world);

		  if ( (strcmp( tq->subject,wildcard1) ==0) || ( (strcmp( tq->subject,wildcard2) ==0) ))
		    {	librdf_statement_set_subject(partial_statement, NULL); 
		    }
		  else 
		    {	librdf_statement_set_subject(partial_statement, librdf_new_node_from_uri_string(p->RDF_world,  tq->subject));	
		    }

		  if ( (strcmp(tq->predicate,wildcard1) ==0) || ( (strcmp(tq->predicate,wildcard2) ==0) ))
		    {	librdf_statement_set_predicate(partial_statement, NULL);
		    }
		  else 
		    {	librdf_statement_set_predicate(partial_statement, librdf_new_node_from_uri_string(p->RDF_world,  tq->predicate));	
		    }



		  if ( (strcmp(tq->object,wildcard1) ==0) || ( (strcmp(tq->object,wildcard2) ==0) ))
		    {	librdf_statement_set_object(partial_statement, NULL); 
		    }
		  else 
		    {	
			  if (! tq->objType) 
			  {
				  librdf_statement_set_object(partial_statement, 
							librdf_new_node_from_uri_string(p->RDF_world, tq->object));
			  }
			  else
			  {						
				  librdf_statement_set_object(partial_statement,
					      		librdf_new_node_from_literal(p->RDF_world, tq->object, NULL, 0));
			  }
		    }


		  if ((p->subs_sheduler_removed_triple ) && (p->subs_sheduler_removed_triple_with_wildcard==FALSE))
		  {	  
			  //fprintf(stderr,"REMOVE triple/s (no wildcards enabled) from triplestore (if present)\n");	  
			  stream=librdf_model_find_statements(p->RDF_model_remove, partial_statement);
		 	  while(!librdf_stream_end(stream)) {
				statement=librdf_stream_get_object(stream);

				//fprintf(stderr,"REMOVE triple from triplestore (if present)\n");
				//librdf_statement_print(statement, stderr);
				//fprintf (stderr,"\n");
				
				if (!librdf_model_context_remove_statement(p->RDF_model_subscribe,context_node ,statement))
				{
					//Really removed
					really_do_query_on_cache=1;
					op->rsp->status = ss_StatusOK;
				}

				librdf_stream_next(stream);

			  }
			  librdf_free_stream(stream);
		  }
		  else if ((p->subs_sheduler_removed_triple ) && (p->subs_sheduler_removed_triple_with_wildcard))
		  {
			  //I cannot filter one time the removing list cause of possible wildcards
			  //fprintf(stderr,"REMOVE triple/s (one or more wildcards enabled) from triplestore (if present)\n");	  
			  stream=librdf_model_as_stream(p->RDF_model_remove);
		 	  while(!librdf_stream_end(stream)) 
			  {
				statement=librdf_stream_get_object(stream);

				sub_node  = librdf_statement_get_subject(statement);
				pred_node = librdf_statement_get_predicate(statement);		
				obj_node  = librdf_statement_get_object(statement);

				//fprintf(stderr,"REMOVE  triple (with or without wildcard) from triplestore\n");
				//librdf_statement_print(statement, stderr);
				//fprintf (stderr,"\n");

				chomped_s=g_strdup(librdf_uri_as_string((librdf_uri *) librdf_node_get_uri(sub_node)));
				chomped_p=g_strdup(librdf_uri_as_string((librdf_uri *) librdf_node_get_uri(pred_node)));

				if (librdf_node_is_literal(obj_node))
				{	
			
					chomped_o=  g_strdup(librdf_node_get_literal_value(obj_node));
				}
				else
				{	
					chomped_o=  g_strdup(librdf_uri_as_string((librdf_uri *) librdf_node_get_uri(obj_node)));
				}


				wildcard_on_remove_s=0;
				wildcard_on_remove_p=0;
				wildcard_on_remove_o=0;

				same_s=0;
				same_p=0;
				same_o=0;


				if ((strcmp(chomped_s,wildcard1) ==0) ||  (strcmp(chomped_s,wildcard2) ==0))
				{
					wildcard_on_remove_s=1;
				}
				else if (
				(strcmp(chomped_s,tq->subject) ==0) || 
				(strcmp(tq->subject,wildcard1) ==0) || 
				(strcmp(tq->subject,wildcard2) ==0)
				)
				{
					same_s=1;
				}


				if ( (strcmp(chomped_p,wildcard1) ==0) ||  (strcmp(chomped_p,wildcard2) ==0))
		   		{
					wildcard_on_remove_p=1;
				}
				else if (
				(strcmp(chomped_p,tq->predicate) ==0) ||
				(strcmp(tq->predicate,wildcard1) ==0) || 
				(strcmp(tq->predicate,wildcard2) ==0)			
				)
				{
					same_p=1;
				}


				if ( (strcmp(chomped_o,wildcard1) ==0) ||  (strcmp(chomped_o,wildcard2) ==0))
				{
					wildcard_on_remove_o=1;
				}
				else if (
				(strcmp(chomped_o,tq->object) ==0)&&((librdf_node_is_literal(obj_node))==(tq->objType)) ||
				(strcmp(tq->object,wildcard1) ==0) || 
				(strcmp(tq->object,wildcard2) ==0)	
				)
				{
					same_o=1;
				}


				if (same_s && same_p && same_o)  
				//No wildcard on statement removed
				{
					//printf("removing triple with no WC (if exist) from context sub\n");
					if (!librdf_model_context_remove_statement(p->RDF_model_subscribe,context_node ,statement))
					{
						//Nothing really removed
						really_do_query_on_cache=1;
						op->rsp->status = ss_StatusOK;
					}

				}
				else if 
				// One, two or three wildcard on the remove statement (needs query on context to update)
				(
				((wildcard_on_remove_s) || (same_s)) &&
				((wildcard_on_remove_p) || (same_p)) &&
				((wildcard_on_remove_o) || (same_o))
				)
				{
					//printf("Wildcard/s in the remove s=%d, p=%d, o=%d\n", wildcard_on_remove_s,wildcard_on_remove_p,wildcard_on_remove_o);
					partial_statement_nested=librdf_new_statement(p->RDF_world);

					if (wildcard_on_remove_s)
						librdf_statement_set_subject(partial_statement_nested, NULL);
					else
						librdf_statement_set_subject(partial_statement_nested, librdf_new_node_from_uri_string(p->RDF_world,  chomped_s));

					if (wildcard_on_remove_p)
						librdf_statement_set_predicate(partial_statement_nested, NULL);
					else
						librdf_statement_set_predicate(partial_statement_nested, librdf_new_node_from_uri_string(p->RDF_world,  chomped_p));

					if (wildcard_on_remove_o)
						librdf_statement_set_object(partial_statement_nested, NULL);
					else
						  if (librdf_node_is_literal(obj_node))
						  {
							  librdf_statement_set_object(partial_statement_nested, 
										librdf_new_node_from_uri_string(p->RDF_world,  chomped_o));
						  }
						  else
						  {						
							  librdf_statement_set_object(partial_statement_nested,
								      		librdf_new_node_from_literal(p->RDF_world, chomped_o, NULL, 0));
						  }

					
					stream_nested=librdf_model_find_statements_in_context(p->RDF_model_subscribe, partial_statement_nested,context_node);
					

				 	while(!librdf_stream_end(stream_nested)) 
					{

						statement_nested=librdf_stream_get_object(stream_nested);

						//fprintf(stderr,"Removing (if exist!) from sub-context triplestore\n");
						//librdf_statement_print(statement_nested, stderr);
						//fprintf (stderr,"\n");
						
						
						if (!librdf_model_context_remove_statement(p->RDF_model_subscribe,context_node ,statement_nested))
						{
							//Nothing really removed
							really_do_query_on_cache=1;
							op->rsp->status = ss_StatusOK;
						}

						librdf_stream_next(stream_nested);
					}
					librdf_free_stream(stream_nested);
					librdf_free_statement(partial_statement_nested);

				}


				g_free(chomped_s);
				g_free(chomped_p);
				g_free(chomped_o);

				librdf_stream_next(stream);

			  }
			  librdf_free_stream(stream);
		  }


		  if (p->subs_sheduler_inserted_triple )
		  {
			  stream=librdf_model_find_statements(p->RDF_model_insert, partial_statement);
		 	  while(!librdf_stream_end(stream)) {
				statement=librdf_stream_get_object(stream);

				//Very annoying!!!.. hope that redland will fix this context problem
				still_present =0;

				stream_nested=librdf_model_find_statements_in_context(p->RDF_model_subscribe, statement,context_node);
				
				if(!librdf_stream_end(stream_nested)) 
				{still_present=1;
				}
				librdf_free_stream(stream_nested);
				//End Very annoying.. 			

				if (!still_present)
				{
					//fprintf(stderr,"ADD new triple to triplestore (if not still present)\n");
					//librdf_statement_print(statement, stderr);
					//fprintf (stderr,"\n");

					librdf_model_context_add_statement(p->RDF_model_subscribe,context_node ,statement);
					
					really_do_query_on_cache=1;
					op->rsp->status = ss_StatusOK;					
				}

				//else
				//{
				//	printf ("Triple still present in this context\n");
				//}

				librdf_stream_next(stream);

			  }
			  librdf_free_stream(stream);
		  }	  

		  librdf_free_statement(partial_statement);



		  REALLY_DO_QUERY_ON_CACHE:
		  if (really_do_query_on_cache)
		  {
		  	  //Time to really do query on volatile cache, in this case I need to take all the sub-context statements

			  
			  stream=librdf_model_context_as_stream(p->RDF_model_subscribe, context_node);
			  while(!librdf_stream_end(stream)) 
		 	  {
				statement=librdf_stream_get_object(stream);

				//librdf_statement_print(statement, stderr);
				//fprintf (stderr,"\n");

				sub_node  = librdf_statement_get_subject(statement);
				pred_node = librdf_statement_get_predicate(statement);		
				obj_node  = librdf_statement_get_object(statement);

			
				ssTriple_t *ttemp = (ssTriple_t *)g_new0(ssTriple_t,1);
				//g_return_val_if_fail(ttemp, ss_NotEnoughResources);

				ttemp->subject=	   g_strdup(librdf_uri_as_string(librdf_node_get_uri(sub_node)));
				ttemp->predicate = g_strdup(librdf_uri_as_string(librdf_node_get_uri(pred_node)));

				if (librdf_node_is_literal(obj_node))
				{	

					ttemp->object =  g_strdup(librdf_node_get_literal_value(obj_node));
					ttemp->objType = ssElement_TYPE_LIT; 
				}
				else
				{	
					
					ttemp->object =  g_strdup(librdf_uri_as_string( librdf_node_get_uri(obj_node)));
					ttemp->objType = ssElement_TYPE_URI; 
				}


				op->rsp->results = g_slist_prepend(op->rsp->results, ttemp);
				librdf_stream_next(stream);

			  }
			  librdf_free_stream(stream);


		  }
		  query_list = query_list->next;
		} //End while

	      librdf_free_node(context_node);
	      break;
	      }  //End case
	    }  //End switch
 
	} // End something added or removed

	  //At least one subcription thread needs to be triggered
	  if (really_do_query_on_cache)
	  {
		  
		  subscription_state* s;
		  g_mutex_lock(p->subscriptions_lock);
		  
		  s = g_hash_table_lookup(p->subs, op->rsp->sub_id);
		  if (NULL != s && s->status != M3_SUB_STOPPED)
		  {
		  s->status = M3_SUB_ONGOING;
		  //printf("QUERY FOR SUBS: Set subscription %s to ongoing\n", s->sub_id); /* SUB_DEBUG */
		  }
		  
		  g_mutex_unlock(p->subscriptions_lock);
		  
		  /////////////////////////////
		  op->op_complete = TRUE;
		  g_cond_signal(op->op_cond);
		  /////////////////////////////  
		  
	  }
	  else
	  {
		  
		  //Trigger for unsub
		  subscription_state* s;
		  g_mutex_lock(p->subscriptions_lock);
		  s = g_hash_table_lookup(p->subs, op->rsp->sub_id);
		  if (NULL != s && s->status == M3_SUB_STOPPED)
		  {
			  g_mutex_unlock(p->subscriptions_lock);
			  /////////////////////////////
			  op->op_complete = TRUE;
			  g_cond_signal(op->op_cond);
			  /////////////////////////////  
		  }
		  else //SUB still active, the persistent query (emulated) will be auto-recharged
		  {
			  g_mutex_unlock(p->subscriptions_lock);
			  g_async_queue_push(p->query_queue, op);
		  }

	  }


}


void do_insert(gpointer op_param, gpointer p_param)
{
  scheduler_item* op = (scheduler_item*) op_param;
  sib_data_structure* p = (sib_data_structure*) p_param;
  switch (op->header->tr_type)
    {
    case M3_INSERT:
      g_mutex_lock(op->op_lock);
      //g_mutex_lock(p->store_lock);
      whiteboard_log_debug("Beginning to insert for transaction %d\n", op->header->tr_id);
      rdf_writer(op, p);
      whiteboard_log_debug("Done inserting for transaction %d\n", op->header->tr_id);
      //g_mutex_unlock(p->store_lock);
      op->op_complete = TRUE;
      // printf("Now signaling transaction %d for finished operation\n", op->header->tr_id);
      g_cond_signal(op->op_cond);
      g_mutex_unlock(op->op_lock);
      break;
    case M3_REMOVE:
      g_mutex_lock(op->op_lock);
      //g_mutex_lock(p->store_lock);
      whiteboard_log_debug("Beginning to remove for transaction %d\n", op->header->tr_id);
      rdf_retractor(op, p);
      whiteboard_log_debug("Done removing for transaction %d\n", op->header->tr_id);
      //g_mutex_unlock(p->store_lock);
      op->op_complete = TRUE;
      g_cond_signal(op->op_cond);
      g_mutex_unlock(op->op_lock);
      break;
    case M3_UPDATE:
      g_mutex_lock(op->op_lock);
      //g_mutex_lock(p->store_lock);
      whiteboard_log_debug("Beginning to update for transaction %d\n", op->header->tr_id);
      rdf_retractor(op, p);
      rdf_writer(op, p);
      whiteboard_log_debug("Done updating for transaction %d\n", op->header->tr_id);
      //g_mutex_unlock(p->store_lock);
      op->op_complete = TRUE;
      g_cond_signal(op->op_cond);
      g_mutex_unlock(op->op_lock);
      break;

      /*AD-ARCES*/
    case M3_PROTECTION_FAULT:
      /* SIB PROTECTION FAULT CASE */
      whiteboard_log_debug("----> SIB PROTECTION FAULT CASE\n");
      g_mutex_lock(op->op_lock);
      /*AD-ARCES*/
      op->rsp->status = ss_SIBProtectionFault;// ss_SIBFailAccessDenied;// ss_InvalidParameter;// ss_OperationFailed;//
      /*AD-ARCES*/
      op->op_complete = TRUE; //FALSE; //it is just for lock purpose
      g_cond_signal(op->op_cond);
      g_mutex_unlock(op->op_lock);
      break;


    default:
      /* ERROR CASE */
      g_mutex_lock(op->op_lock);
      op->rsp->status = ss_InvalidParameter;
      op->op_complete = TRUE;
      g_cond_signal(op->op_cond);
      g_mutex_unlock(op->op_lock);
      break;
    }
}

void do_query(gpointer op_param, gpointer p_param)
{
  scheduler_item* op = (scheduler_item*) op_param;
  sib_data_structure* p = (sib_data_structure*) p_param;
  switch (op->header->tr_type)
    {
    /* Query and subscribe handled similarly at this level (for now) */
    case M3_SUBSCRIBE:
      /* Fallthrough */
    case M3_QUERY: //and subscribe
      g_mutex_lock(op->op_lock);
      //g_mutex_lock(p->store_lock);
      //g_mutex_lock(p->temp_ins_rem_operations_lock);
      //whiteboard_log_debug("Beginning to query for transaction %d\n", op->header->tr_id);
      rdf_reader(op,p);
      //whiteboard_log_debug("Done querying for transaction %d\n", op->header->tr_id);
      //g_mutex_unlock(p->store_lock);
      //g_mutex_unlock(p->temp_ins_rem_operations_lock);
      
      g_mutex_unlock(op->op_lock);
      //printf("processing query");
      break;
    default:
      /* ERROR CASE */
      g_mutex_lock(op->op_lock);
      op->rsp->status = ss_InvalidParameter;
      op->op_complete = TRUE;
      g_cond_signal(op->op_cond);
      g_mutex_unlock(op->op_lock);
      break;
    }
}

void clean_temp_ins_rem_triplestores(gpointer data)
{
	librdf_statement*	statement;
	librdf_stream* 		stream;

	sib_data_structure* p = (sib_data_structure*)data;

	//CLEANING TEMP INSERT REMOVE REGISTER
	whiteboard_log_debug("Start_clean_temp_ins_rem_TS\n");


	//g_mutex_lock(p->temp_ins_rem_operations_lock);
	stream=librdf_model_as_stream(p->RDF_model_insert);
	while(!librdf_stream_end(stream))
	{
		statement=librdf_stream_get_object(stream);
		librdf_model_remove_statement(p->RDF_model_insert, statement);
		librdf_stream_next(stream);
	}
	librdf_free_stream(stream);


	stream=librdf_model_as_stream(p->RDF_model_remove);
	while(!librdf_stream_end(stream))
	{
		statement=librdf_stream_get_object(stream);
		librdf_model_remove_statement(p->RDF_model_remove, statement);
		librdf_stream_next(stream);
	}
	librdf_free_stream(stream);
	//g_mutex_unlock(p->temp_ins_rem_operations_lock);


	/*
	librdf_free_model(p->RDF_storage_insert );
	librdf_free_model(p->RDF_storage_remove);

	p->RDF_model_insert=librdf_new_model(p->RDF_world, p->RDF_storage_insert , NULL);
	p->RDF_model_remove=librdf_new_model(p->RDF_world, p->RDF_storage_remove , NULL);
	*/

	p->subs_sheduler_inserted_triple= FALSE;
	p->subs_sheduler_removed_triple=  FALSE;
	p->subs_sheduler_removed_triple_with_wildcard=FALSE;

    ////////////////////////
	whiteboard_log_debug("End_clean_temp_ins_rem_TS\n");
	////////////////////////
}

void set_sub_to_pending(gpointer sub_id, gpointer sub_data, gpointer unused)
{
  subscription_state* s = (subscription_state*)sub_data;

  if (s->status != M3_SUB_STOPPED)
    s->status = M3_SUB_PENDING;

  whiteboard_log_debug("Set subscription %s to pending\n", s->sub_id); /* SUB_DEBUG */

}

void set_sub_to_ongoing(gpointer sub_id, gpointer sub_data, gpointer unused)
{
  subscription_state* s = (subscription_state*)sub_data;
  if (s->status != M3_SUB_STOPPED)
    s->status = M3_SUB_ONGOING;
  whiteboard_log_debug("Set subscription %s to ongoing\n", s->sub_id); /* SUB_DEBUG */
}

/*
 * Scheduler (or serializer) to read operation requests from insert and query
 * queue one at a time and call corresponding function to handle the
 * communication.
 *
 * When new inserts have been made, read contents of insert and query queues
 * to the corresponding lists and process these 1. insert 2. query
 * Check that subscriptions do not miss any inserts
 */

gpointer scheduler(gpointer data)
{
  sib_data_structure* p = (sib_data_structure*)data;

  GAsyncQueue* i_queue = p->insert_queue;
  GAsyncQueue* q_queue = p->query_queue;

  GAsyncQueue* sub_context_queue = p->clean_context_sub;

  GCond* new_reqs_cond = p->new_reqs_cond;
  GMutex* new_reqs_lock = p->new_reqs_lock;

  GHashTable* subs = p->subs;
  GMutex* subscriptions_lock = p->subscriptions_lock;


  gboolean updated = false;

  GSList* i_list = NULL;
  GSList* q_list = NULL;

  GSList* clean_context_list = NULL;

   
  scheduler_item* op;

  gchar* sub_id;


  while (TRUE) {

    /* Wait until there is actually something in the queues */
    g_mutex_lock(new_reqs_lock);
    while (!(p->new_reqs))
      {
	g_cond_wait(new_reqs_cond, new_reqs_lock);
      } 
    p->new_reqs = FALSE;
    g_mutex_unlock(new_reqs_lock);

    whiteboard_log_debug("Schedule Cycle Begin\n");

    /* Lock insert and query queues
       Insert contents into lists for processing
    */

    g_async_queue_lock(i_queue);
    while (NULL !=
	   (op = (scheduler_item*)g_async_queue_try_pop_unlocked(i_queue)))
      {

	if (!p->disable_protection)
	{
		/*AD-ARCES*/
	    	whiteboard_log_debug("#######################################\n");
	    	whiteboard_log_debug("PROTECTION CONTROL \n");
	    	whiteboard_log_debug("#######################################\n");

	    	//LCTable_Test();
	    	ProtectionCompatibilityFilter(op);
	}

	i_list = g_slist_prepend(i_list, op);
	whiteboard_log_debug("Added item to insert list");
      }
    g_async_queue_unlock(i_queue);

    if (i_list != NULL)
      updated = true;

    g_async_queue_lock(q_queue);
    while (NULL !=
	   (op = (scheduler_item*)g_async_queue_try_pop_unlocked(q_queue)))
      {
	q_list = g_slist_prepend(q_list, op);
	whiteboard_log_debug("Added item to query list");
      }
    g_async_queue_unlock(q_queue);


    //Clean Redland Temp Ins/Rem Triplestores 
    clean_temp_ins_rem_triplestores(p);


    g_slist_foreach(i_list, do_insert, p);
    g_slist_free(i_list);
    i_list = NULL;

    if (updated)
      {
	g_mutex_lock(subscriptions_lock);
	g_hash_table_foreach(subs, set_sub_to_pending, NULL);
	g_mutex_unlock(subscriptions_lock);
	updated = false;
	//printf("RDF store updated, set all subscriptions to pending\n"); /* SUB_DEBUG */
      }


    g_slist_foreach(q_list, do_query, p);
    g_slist_free(q_list);
    q_list = NULL;


    /* Process plugin reasoners here */
    // ..or not..
    /* Plugin End		     */


    //Clean ended subsciption contexts
    g_async_queue_lock(sub_context_queue);
    while (NULL !=
	   (sub_id = g_async_queue_try_pop_unlocked(sub_context_queue)))
      {
	clean_context_list = g_slist_prepend(clean_context_list, sub_id);
      }
    g_async_queue_unlock(sub_context_queue);

    g_slist_foreach(clean_context_list, clean_context_in_sheduler, p);
    g_slist_free(clean_context_list);
    clean_context_list = NULL;

  }


}

/*
 * Initialization of SIB internal data structures
 * Contains
 * - mutexes for KP membership, subscription state and
 *   RDF store access
 * - Hash tables for joined KPs and ongoing subscriptions
 * - Queues for insertions and deletions
 *
*/

sib_data_structure* sib_initialize(gchar* name, gpointer parameter_list)
{

  sib_data_structure* sd;

  GSList* param_list = parameter_list;
  /* Allocate sib data structures */

  sd = g_new0(sib_data_structure, 1);
  if (NULL == sd) exit(-1);

  /* Initialize smart space name */
  sd->ss_name = name;

  sd->insert_queue = g_async_queue_new();
  if (NULL == sd->insert_queue) exit(-1);

  sd->query_queue = g_async_queue_new();
  if (NULL == sd->query_queue) exit(-1);

  sd->clean_context_sub = g_async_queue_new();
  if (NULL == sd->clean_context_sub) exit(-1);

  sd->joined = g_hash_table_new(g_str_hash, g_str_equal);
  if (NULL == sd->joined) exit(-1);

  sd->subs = g_hash_table_new(g_str_hash, g_str_equal);
  if (NULL == sd->subs) exit(-1);

  //sd->members_lock = g_mutex_new();
  //if (NULL == sd->members_lock) exit(-1);

  sd->subscriptions_lock = g_mutex_new();
  if (NULL == sd->subscriptions_lock) exit(-1);

  //sd->store_lock = g_mutex_new();
  //if (NULL == sd->store_lock) exit(-1);

  sd->new_reqs_lock = g_mutex_new();
  if (NULL == sd->new_reqs_lock) exit(-1);

  sd->new_reqs_cond = g_cond_new();
  if (NULL == sd->new_reqs_cond) exit(-1);

  //sd->store_subscribe_lock = g_mutex_new();
  //if (NULL == sd->new_reqs_lock) exit(-1);

  sd->new_reqs = FALSE;

  /* Start scheduler */
  g_thread_create(scheduler, sd, FALSE, NULL);

  //INITIALIZE REDLAND ENVIROMENT
  sd->RDF_world=librdf_new_world();
  librdf_world_open(sd->RDF_world);

  //CHECK FOR PARAMETERS
  sd->enable_rdf_pp	= 	FALSE;
  sd->disable_protection = 	FALSE;
  gboolean mem_volatile      = 	FALSE;
  gboolean sqlite =	 	FALSE;
  gboolean subs_persistent   =  FALSE;


  for ( ; param_list != NULL ; param_list = g_slist_next(param_list))
  {
	gchar* current_parameter= param_list->data;
	if (strcmp ((gchar*)"--disable-protections",(gchar*)current_parameter)==0)
	{
		 printf("Launching with no protections option\n");
		 sd->disable_protection = TRUE;
	}
	if (strcmp ((gchar*)"--storage-volatile",(gchar*)current_parameter)==0)
	{
		 printf("Launching with no persistent DB\n");
		 mem_volatile = TRUE;
	}
	if (strcmp ((gchar*)"--storage-sqlite",(gchar*)current_parameter)==0)
	{
		 printf("Launching with Sqlite storage (SLOW!)\n");
		 sqlite = TRUE;
	}
	if (strcmp ((gchar*)"--storage-subs-persistent",(gchar*)current_parameter)==0)
	{
		 printf("Launching with persistent subscription database\n");
		 subs_persistent = TRUE;
	}
	if (strcmp ((gchar*)"--enable-rdf++",(gchar*)current_parameter)==0)
	{
		 printf("Launching Reasoner. No persistent DB will be rescued !\n");
		 sd->enable_rdf_pp = TRUE;
	}


  }

  //MAIN TRIPLESTORE
  if (mem_volatile)
  {
  	sd->RDF_storage=librdf_new_storage(sd->RDF_world, "hashes", NULL, "hash-type='memory'");
	//sd->RDF_storage=librdf_new_storage(sd->RDF_world,  NULL, NULL, NULL);
  }
  else
  {
	if (sqlite)
  	{
		if ((file_exists(sd->ss_name)) && (sd->enable_rdf_pp == FALSE))
		{
			printf("Found a local sqlite db, re-charging.. \n");
			sd->RDF_storage=librdf_new_storage(sd->RDF_world, "sqlite", sd->ss_name, NULL);
		}
		else
		{	printf("Creating a new Sqlite db.. \n");
			sd->RDF_storage=librdf_new_storage(sd->RDF_world, "sqlite", sd->ss_name, "new='yes'");
		}
	}
	else
	{
		
		//BDB default

		gchar* sp2o;
		gchar* po2s;
		gchar* so2p;

		sp2o= g_strdup_printf("%s%s",sd->ss_name,"-sp2o.db");
		so2p= g_strdup_printf("%s%s",sd->ss_name,"-so2p.db");
		po2s= g_strdup_printf("%s%s",sd->ss_name,"-po2s.db");


		if ((
		   (file_exists( sp2o)) &&
		   (file_exists( so2p)) &&
		   (file_exists( po2s))
		   ) && (sd->enable_rdf_pp == FALSE))
		{
			printf("Found a BDB local file, re-charging.. \n");
			sd->RDF_storage=librdf_new_storage(sd->RDF_world, "hashes", sd->ss_name, "new='no',hash-type='bdb',dir='.'");
			
		}
		else
		{
			sd->RDF_storage=librdf_new_storage(sd->RDF_world, "hashes", sd->ss_name, "new='yes',hash-type='bdb',dir='.'");
		}
		
	}
  }

  sd->RDF_model=librdf_new_model(sd->RDF_world, sd->RDF_storage , NULL);

  //ARCES DIFFERENTIAL SUBS INS/REM SCHEDULER TRIPLESTORES//////////////////////////////////////////////////////////
  
  //ALTERNATE
  //sd->RDF_storage_insert=librdf_new_storage(sd->RDF_world, "hashes", NULL, "hash-type='memory'");
  sd->RDF_storage_insert=librdf_new_storage(sd->RDF_world, NULL, NULL, NULL);

  sd->RDF_model_insert=librdf_new_model(sd->RDF_world, sd->RDF_storage_insert , NULL);

  //ALTERNATE
  //sd->RDF_storage_remove=librdf_new_storage(sd->RDF_world, "hashes", NULL, "hash-type='memory'");
  sd->RDF_storage_remove=librdf_new_storage(sd->RDF_world, NULL, NULL, NULL);

  sd->RDF_model_remove=librdf_new_model(sd->RDF_world, sd->RDF_storage_remove , NULL);

  //INITIALIZE REDLAND SUBS CONTEXTS TRIPLESTORE//////////////////////////////////////////////////////////////
  if (!subs_persistent)
  {
  //sd->RDF_storage_subscribe=librdf_new_storage(sd->RDF_world, "hashes", NULL, "contexts='yes',hash-type='memory'");
  sd->RDF_storage_subscribe=librdf_new_storage(sd->RDF_world, NULL, NULL, "contexts='yes'");
  }
  else
  {
  sd->RDF_storage_subscribe=librdf_new_storage(sd->RDF_world, "hashes", strcat(sd->ss_name,(char *)"_subs"),"contexts='yes',new='yes',hash-type='bdb',dir='.'");
  }
  sd->RDF_model_subscribe=librdf_new_model(sd->RDF_world, sd->RDF_storage_subscribe , NULL);
  //////////////////////////////////////////////////////////////////////////////////////////////////////////
  sd->temp_ins_rem_operations_lock = g_mutex_new();


  ////REASONING INIT
  sd->subClasses = 	g_hash_table_new_full(g_str_hash, g_str_equal,  g_free, NULL);
  sd->subProperties = 	g_hash_table_new_full(g_str_hash, g_str_equal,  g_free, NULL);
  sd->propertyDomain= 	g_hash_table_new_full(g_str_hash, g_str_equal,  g_free, NULL);
  sd->propertyRange= 	g_hash_table_new_full(g_str_hash, g_str_equal,  g_free, NULL);
  ////--REASONING

  if (NULL == sd->new_reqs_cond) exit(-1);
  if (NULL == sd->RDF_model) exit(-1);

  return sd;
}
