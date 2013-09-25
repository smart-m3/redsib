/*
  2009, Nokia Corporation
  2012-13, ARCES, University of Bologna
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
#include "core_utilities.h"
#include "sparql_booster.h"

#include <sys/time.h>
#include <uuid/uuid.h>


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

#define subs_uri	"http://subscribe_graph#"


extern void ssFreeTriple (ssTriple_t *triple);
extern void ssFreeTripleList (GSList **tripleList);

extern gboolean live_debug_main = FALSE;
extern gboolean live_debug_sparql_booster = FALSE;




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
		// whiteboard_log_debug("Now signaling transaction %d for finished operation\n", op->header->tr_id);
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


void do_query_subscribe(gpointer op_param, gpointer p_param)
{
	scheduler_item* op = (scheduler_item*) op_param;
	sib_data_structure* p = (sib_data_structure*) p_param;


	switch (op->header->tr_type)
	{
	/* Query and subscribe handled similarly at this level (for now) */
	case M3_SUBSCRIBE:
		if (op->rsp->first_subscribe)
		{
			//For the first subscribe a complete query is needed
			g_mutex_lock(op->op_lock);
			//Generating sub TS

			if (op->rsp->sparql_subscribe==TRUE)
			{
				// Necessary to run firstly as a template to create a triple context for
				// the first sparql result (probably the faster solution)
				//////////////////////////////////////
				op->req->type=QueryTypeTemplate;
				rdf_reader(op,p);
				op->req->type=QueryTypeSPARQLSelect;
				//////////////////////////////////////
			}
			else
			{
				rdf_reader(op,p);
			}

			op->rsp->first_subscribe=FALSE;
			threadsub_wake_and_reload_op(op,p);
			g_mutex_unlock(op->op_lock);
		}
		else
		{	//Common query
			g_mutex_lock(op->op_lock);
			rdf_subscribe(op,p);
			g_mutex_unlock(op->op_lock);
		}
		break;
	case M3_QUERY_SPQL_UPD:
		g_mutex_lock(op->op_lock);
		rdf_reader(op,p);
		/////////////////////////////
		op->op_complete = TRUE;
		g_cond_signal(op->op_cond);
		/////////////////////////////
		g_mutex_unlock(op->op_lock);
		//whiteboard_log_debug("processing query");
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
			// whiteboard_log_debug("INSERT: parsed bnodes str: %s\n", rsp_msg->bnodes_str);
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
		whiteboard_log_debug("COULD NOT PARSE INSERT DBUS MESSAGE\n");
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
		whiteboard_log_debug("COULD NOT PARSE REMOVE DBUS MESSAGE\n");
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



gpointer m3_query_spql_upd(gpointer data)
{
	ssap_message_header *header;
	ssap_kp_message *req_msg;
	ssap_sib_message *rsp_msg;
	GMutex* op_lock;
	GCond* op_cond;
	scheduler_item* s;
	ssStatus_t status;
	sib_op_parameter* param = (sib_op_parameter*) data;

	gboolean  is_sparql_update =FALSE;

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
		header->tr_type = M3_QUERY_SPQL_UPD;
		header->msg_type = M3_REQUEST;
		rsp_msg->status = ss_StatusOK;
		switch(req_msg->type)
		{
			case QueryTypeTemplate:
				status = parseM3_triples(&(req_msg->template_query),
						 req_msg->query_str,
						 NULL);
				break;
			case QueryTypeSPARQLSelect:
				//whiteboard_log_debug("SPARQL!\n");
				is_sparql_update=check_sparql_is_update(req_msg->query_str);
				status = ss_StatusOK; //FOR NOW!
				break;
			default: // Error //
				whiteboard_log_debug("UNREC!");
				rsp_msg->status = ss_SIBFailNotImpl;
				rsp_msg->results_str = g_strdup("");
				goto send_response;
				break;
		}

		if (! is_sparql_update)
		{

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
						ssFreeTripleList(&(req_msg->template_query));
						rsp_msg->results_str = m3_gen_triple_string(rsp_msg->results);
						m3_free_triple_list_simple(&(rsp_msg->results));
						break;
					case QueryTypeSPARQLSelect:
						req_msg->template_query=NULL;
						break;
					default:
						whiteboard_log_debug("UNREC!");
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

				g_free(rsp_msg->results_str);

				g_mutex_free(op_lock);
				g_cond_free(op_cond);
				g_free(s);
				//g_free(rsp_msg->results_str);
				g_free(req_msg);
				g_free(rsp_msg);
				g_free(header);
		}
		else
		{
			//SPARQL UPDATE
			s->header = header;
			s->req = req_msg;
			s->rsp = rsp_msg;
			s->op_lock = op_lock;
			s->op_cond = op_cond;
			s->op_complete = FALSE;

			////////////////////////////////

			gboolean  insert_syntax_error;
			gboolean  remove_syntax_error;

			gboolean  insert_query_results;
			gboolean  remove_query_results;

			gboolean  delayed_update ;
			gboolean  delayed_update_outoftime ;

			gboolean series_update_followers = FALSE;

		    gchar** split_update_array;
		    gchar** pointer_split_update_array;

		    gchar * temp_query= g_strdup(req_msg->query_str);
		    split_update_array=g_strsplit( req_msg->query_str , sparql_seq_separator , 0);
		    pointer_split_update_array = split_update_array;

		    while (*pointer_split_update_array)
		    {

				insert_syntax_error = FALSE;
				remove_syntax_error = FALSE;

				insert_query_results = FALSE;
				remove_query_results = FALSE;

				delayed_update = FALSE;
				delayed_update_outoftime = FALSE;

				gchar * remove_construct;
				//remove_construct= get_remove_construct(req_msg->query_str);
				remove_construct= get_remove_construct(*pointer_split_update_array);
				//if (remove_construct)
					//printf ("%s\n", remove_construct);


				gchar * insert_construct;
				insert_construct= get_insert_construct(*pointer_split_update_array);
				//if (insert_construct)
					//printf ("%s\n", insert_construct);


				if  (insert_construct || remove_construct)
				{

					double sparql_time_value =0;
					sparql_time_value = get_sparql_update_scheduled_time (*pointer_split_update_array);
					//sparql_time_value = get_sparql_update_scheduled_time (req_msg->query_str);

					struct timeval tv;
					gettimeofday(&tv, 0);
					double realtime= (int)tv.tv_sec + ((double)(int)tv.tv_usec / 1000000);

					if ((sparql_time_value > 0) && (sparql_time_value - realtime > 0))
					{
						//DELAYED IN THE FUTURE
						if (!series_update_followers)
						{

							rsp_msg->results_str=g_strdup(scheduler_upd_resp_true);
							rsp_msg->status=ss_StatusOK;

							whiteboard_util_send_method_return(param->conn,
									param->msg,
									DBUS_TYPE_STRING, &(header->space_id),
									DBUS_TYPE_STRING, &(header->kp_id),
									DBUS_TYPE_INT32, &(header->tr_id),
									DBUS_TYPE_INT32, &(rsp_msg->status),
									DBUS_TYPE_STRING, &(rsp_msg->results_str),
									WHITEBOARD_UTIL_LIST_END);


							g_free(rsp_msg->results_str);

						}

						usleep((int)((sparql_time_value - realtime)*1000000));
						gboolean  delayed_update = TRUE;


					}
					else if ((sparql_time_value > 0) && (sparql_time_value - realtime < 0))
					{
						//INTERVAL EXPIRED!  SENDING FALSE AND TERMINATING THE THREAD


						//CHOICES:

						//SENDING FALSE AND TERMINATING THE THREAD
						/*
						 *
						 *
						rsp_msg->results_str=g_strdup(sparql_upd_resp_false);
						rsp_msg->status=ss_StatusOK;

						whiteboard_util_send_method_return(param->conn,
								param->msg,
								DBUS_TYPE_STRING, &(header->space_id),
								DBUS_TYPE_STRING, &(header->kp_id),
								DBUS_TYPE_INT32, &(header->tr_id),
								DBUS_TYPE_INT32, &(rsp_msg->status),
								DBUS_TYPE_STRING, &(rsp_msg->results_str),
								WHITEBOARD_UTIL_LIST_END);


						g_free(rsp_msg->results_str);

						g_free(remove_construct);
						g_free(insert_construct);

						g_mutex_free(op_lock);
						g_cond_free(op_cond);

						g_free(req_msg);
						g_free(rsp_msg);
						g_free(header);
						g_free(s);

						dbus_message_unref(param->msg);
						g_free(param);

						return NULL;
						*/


						//  DO UPDATE EVEN IF OUT OF TIME ..
						//  Do nothing and proceed normally.. Just sending false result
						delayed_update_outoftime=TRUE;
					}
				}

				///
				g_mutex_lock(param->sib->sparql_update_lock);
				///

				m3_free_triple_list_simple(&(param->sib->RDF_list_insert));
				m3_free_triple_list_simple(&(param->sib->RDF_list_remove));

				if (remove_construct)
				{

					librdf_query * query;
					librdf_query_results * results;
					librdf_stream * stream;
					librdf_statement * statement;

					gchar * remove_construct_timed = convert_sib_time_calls( remove_construct);
					query=librdf_new_query(param->sib->RDF_world, "sparql", NULL, remove_construct_timed, NULL);
					results=librdf_model_query_execute(param->sib->RDF_model, query);
					g_free(remove_construct_timed);

					if (results == NULL)
					{
						remove_syntax_error=TRUE;
						//printf("SPARQL syntax error! \n");
						librdf_free_query_results(results);
						librdf_free_query(query);
					}
					else if (librdf_query_results_is_graph(results))
					{
						stream=librdf_query_results_as_stream(results);
						while(!librdf_stream_end(stream))
						{
							remove_query_results=TRUE;
							param->sib->subs_sheduler_removed_triple=TRUE;

							statement=librdf_stream_get_object(stream);
							////////////////
							statement_in_tl(&(param->sib->RDF_list_remove), statement);
							////////////////
							librdf_stream_next(stream);
						}
						librdf_free_stream(stream);

					}
					librdf_free_query_results(results);
					librdf_free_query(query);
				}
				if (insert_construct)
				{

					librdf_query * query;
					librdf_query_results * results;
					librdf_stream * stream;
					librdf_statement * statement;

					gchar * insert_construct_timed = convert_sib_time_calls( insert_construct);
					query=librdf_new_query(param->sib->RDF_world, "sparql", NULL, insert_construct_timed, NULL);
					results=librdf_model_query_execute(param->sib->RDF_model, query);
					g_free(insert_construct_timed);

					if (results == NULL)
					{
						insert_syntax_error=TRUE;
						//printf("SPARQL syntax error! \n");
						librdf_free_query_results(results);
						librdf_free_query(query);
					}
					else if (librdf_query_results_is_graph(results))
					{
						stream=librdf_query_results_as_stream(results);

						while(!librdf_stream_end(stream))
						{
							insert_query_results=TRUE;
							param->sib->subs_sheduler_inserted_triple=TRUE;

							statement=librdf_stream_get_object(stream);
							////////////////
							statement_in_tl(&(param->sib->RDF_list_insert), statement);
							////////////////
							librdf_stream_next(stream);
						}
						librdf_free_stream(stream);
					}
					librdf_free_query_results(results);
					librdf_free_query(query);

				}

				// UPDATING TS to REMOVE LIST

				GSList * pointer = param->sib->RDF_list_remove;
				librdf_statement * statement;

				while (pointer)
				{
					ssTriple_t * t = pointer->data;
					//Insert statement
					statement=librdf_new_statement(param->sib->RDF_world);

					librdf_statement_set_subject(statement,
						librdf_new_node_from_uri_string(param->sib->RDF_world,  t->subject));

					librdf_statement_set_predicate(statement,
						librdf_new_node_from_uri_string(param->sib->RDF_world, t->predicate));

					if (! t->objType) {
						//URI: 	t->objType==0
						librdf_statement_set_object(statement,
							librdf_new_node_from_uri_string(param->sib->RDF_world,  t->object));
					}
					else
					{	//Literal:  	t->objType==1
						librdf_statement_set_object(statement,
							librdf_new_node_from_literal(param->sib->RDF_world, t->object, NULL, 0));
					}

					if ( param->sib->virtuoso )
						librdf_model_context_remove_statement(param->sib->RDF_model, param->sib->virtuoso_context, statement);
					else
						librdf_model_remove_statement(param->sib->RDF_model, statement);

					librdf_free_statement(statement);

					pointer=pointer->next;
				}

				// UPDATING TS to INSERT LIST

				pointer = param->sib->RDF_list_insert;
				while (pointer)
				{
					ssTriple_t * t = pointer->data;
					//Insert statement
					statement=librdf_new_statement(param->sib->RDF_world);

					librdf_statement_set_subject(statement,
						librdf_new_node_from_uri_string(param->sib->RDF_world,  t->subject));

					librdf_statement_set_predicate(statement,
						librdf_new_node_from_uri_string(param->sib->RDF_world, t->predicate));

					if (! t->objType) {
						//URI: 	t->objType==0
						librdf_statement_set_object(statement,
							librdf_new_node_from_uri_string(param->sib->RDF_world,  t->object));
					}
					else
					{
						//Literal:  	t->objType==1
						librdf_statement_set_object(statement,
							librdf_new_node_from_literal(param->sib->RDF_world, t->object, NULL, 0));
					}

					if ( param->sib->virtuoso)
						librdf_model_context_add_statement(param->sib->RDF_model, param->sib->virtuoso_context, statement);
					else
						librdf_model_add_statement(param->sib->RDF_model, statement);

					librdf_free_statement(statement);

					//Reasoning
					reasoning(param->sib, t, param->sib->enable_rdf_pp);

					pointer=pointer->next;
				}


				////////////////////////////////////
				//   	IF TRIPLES INS / RM ->    //
				//	 		SCHEDULER EMULATOR	  //
				////////////////////////////////////

				if (param->sib->RDF_list_insert || param->sib->RDF_list_remove)
				{

					gpointer * op;
					GSList * q_s_list=NULL;
					GHashTable* subs = param->sib->subs;
					GMutex* subscriptions_lock = param->sib->subscriptions_lock;

					g_async_queue_lock(param->sib->query_queue);
					while (NULL !=
						(op = (scheduler_item*)
							  g_async_queue_try_pop_unlocked(param->sib->query_queue)))
					{
						q_s_list = g_slist_prepend(q_s_list, op);
						whiteboard_log_debug("Added item to query list");
					}
					g_async_queue_unlock(param->sib->query_queue);

					g_mutex_lock(subscriptions_lock);
					g_hash_table_foreach(subs, set_sub_to_pending, NULL);
					g_mutex_unlock(subscriptions_lock);

					g_slist_foreach(q_s_list, do_query_subscribe, param->sib);
					g_slist_free(q_s_list);
					q_s_list = NULL;

					/////////////////////////////////

				}
				g_mutex_unlock(param->sib->sparql_update_lock);



				if ((!delayed_update) && (!series_update_followers))
				{

						if  (
								(insert_construct || remove_construct) &&
								(!insert_syntax_error && !remove_syntax_error )
						)
						{
							if (delayed_update_outoftime)
								rsp_msg->results_str=g_strdup(scheduler_upd_resp_false); //FOR NOW..
							else
							{
								if (insert_query_results || remove_query_results)
									rsp_msg->results_str=g_strdup(sparql_upd_resp_true);  //FOR NOW
								else
									rsp_msg->results_str=g_strdup(sparql_upd_resp_false); //FOR NOW
							}
							rsp_msg->status=ss_StatusOK;
						}
						else
						{
							rsp_msg->results_str=g_strdup_printf("");
							rsp_msg->status = ss_GeneralError;

						}


						whiteboard_util_send_method_return(param->conn,
								param->msg,
								DBUS_TYPE_STRING, &(header->space_id),
								DBUS_TYPE_STRING, &(header->kp_id),
								DBUS_TYPE_INT32, &(header->tr_id),
								DBUS_TYPE_INT32, &(rsp_msg->status),
								DBUS_TYPE_STRING, &(rsp_msg->results_str),
								WHITEBOARD_UTIL_LIST_END);

						g_free(rsp_msg->results_str);

				}



				g_free(remove_construct);
				g_free(insert_construct);

				series_update_followers=TRUE;
				pointer_split_update_array ++;

		    }

		    g_free(temp_query);
		    g_strfreev (split_update_array);


			g_mutex_free(op_lock);
			g_cond_free(op_cond);

			g_free(req_msg);
			g_free(rsp_msg);
			g_free(header);

			g_free(s);

		}
	}
	else
	{
		whiteboard_log_warning("Could not parse QUERY method call message\n");
	}

	dbus_message_unref(param->msg);
	g_free(param);


	return NULL;

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
	subscription_state *sub_state;
    gboolean sub_ts_has_changed ;

    GMutex* thread_loop_lock;

	gboolean sub_processing_on_thread_local=TRUE;


	space_id = header->space_id;
	kp_id = header->kp_id;
	tr_id = header->tr_id;

	s = g_new0(scheduler_item, 1);
	op_lock = g_mutex_new();
	op_cond = g_cond_new();
    thread_loop_lock =  g_mutex_new();


	s->header = header;
	s->req = req_msg;
	s->rsp = rsp_msg;
	s->op_lock = op_lock;
	s->op_cond = op_cond;
	s->op_complete = FALSE;
	s->thread_loop_lock=thread_loop_lock;

	s->rsp->param= param;

	g_async_queue_push(param->sib->query_queue, s);

	// Signal scheduler that new operation has been added to queue
	g_mutex_lock(param->sib->new_reqs_lock);
	param->sib->new_reqs = TRUE;
	// whiteboard_log_debug("Now signaling SCHEDULER in SUBSCRIBE tr %d\n", header->tr_id);
	g_cond_signal(param->sib->new_reqs_cond);
	g_mutex_unlock(param->sib->new_reqs_lock);

	// Block while operation is being processed
	g_mutex_lock(s->op_lock);
	while (!(s->op_complete))
	{
		g_cond_wait(s->op_cond, s->op_lock);
	}
	s->op_complete = FALSE;
	g_mutex_unlock(op_lock);
	//whiteboard_log_debug("Got baseline query result for subscription %s\n", rsp_msg->sub_id);


	rsp_msg->results_str = m3_gen_triple_string(rsp_msg->results);
	m3_free_triple_list_simple(&(rsp_msg->results));

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

	rsp_msg->results=NULL;

    do
    {

        //whiteboard_log_debug("Loop sub thread %s  \n", sub_id);

        // Check if unsubscribed //
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
            // UNLOCK SUBSCRIPTION TABLE /
            g_mutex_unlock(param->sib->subscriptions_lock);
            // UNLOCK UNSUB LOCK //

            g_free(rsp_msg->sparql_query_str);

            g_mutex_free(thread_loop_lock);
            g_cond_free(op_cond);
            g_mutex_free(op_lock);
            g_free(s);

            break;
        }

        g_mutex_lock(param->sib->subscriptions_lock);
        sub_state = (subscription_state*)g_hash_table_lookup(param->sib->subs, sub_id);
        g_mutex_unlock(param->sib->subscriptions_lock);

        if (sub_state->status == M3_SUB_PENDING)
        {
            g_mutex_lock(param->sib->new_reqs_lock);
            param->sib->new_reqs = TRUE;
            g_cond_signal(param->sib->new_reqs_cond);
            g_mutex_unlock(param->sib->new_reqs_lock);
            //whiteboard_log_debug("Subscription %s pending, setting new_reqs flag\n", rsp_msg->sub_id); // SUB_DEBUG //
        }

        // Block while operation is processed //
        g_mutex_lock(s->thread_loop_lock);
        s->thread_busy=FALSE;
        g_mutex_unlock(s->thread_loop_lock);

        //g_cond_signal(s->thread_is_free);

        g_mutex_lock(s->op_lock);
        while (!(s->op_complete))
        {
            g_cond_wait(s->op_cond, s->op_lock);
        }
        s->op_complete = FALSE;
        g_mutex_unlock(op_lock);

        g_mutex_lock(s->thread_loop_lock);
        s->thread_busy=TRUE;
        g_mutex_unlock(s->thread_loop_lock);

        whiteboard_log_debug("Got new subscription result for transaction %d\n", tr_id);
        //whiteboard_log_debug("Got new query result for subscription %s\n", rsp_msg->sub_id); //

	    g_mutex_lock(rsp_msg->sub_processing_on_thread_lock);
	    sub_processing_on_thread_local=rsp_msg->sub_processing_on_thread;
	    g_mutex_unlock(rsp_msg->sub_processing_on_thread_lock);

	    if (sub_processing_on_thread_local)
	    {

	        ////////////////////
			g_mutex_lock(param->sib->sub_threads_lock);
			//printf("full active th: %d ", param->sib->sub_threads_active);
			if (param->sib->sub_threads_active  <  param->sib->sub_threads_max)
			    param->sib->sub_threads_active ++;
			g_mutex_unlock(param->sib->sub_threads_lock);
			////////////////////

			gboolean result;
			result= load_rdf_subscriptions(s, param->sib);
			//printf("Loaded on thread, results found : %d\n", result);

            ////////////////////
            g_mutex_lock(param->sib->sub_threads_lock);
            if (param->sib->sub_threads_active  > 0)
                param->sib->sub_threads_active --;
            g_mutex_unlock(param->sib->sub_threads_lock);
            ////////////////////

	    }	//END FULL THREAD CASE


    }
    while(TRUE);

    return ss_StatusOK;
}


gint m3_subscribe_sparql(ssap_message_header *header,ssap_kp_message *req_msg, ssap_sib_message *rsp_msg, sib_op_parameter* param){

	GMutex* op_lock;
	GCond* op_cond;
	scheduler_item* s;

	gchar *space_id, *kp_id;
	gchar* sub_id = rsp_msg->sub_id;
	gint tr_id;

	subscription_state *sub_state;

	gboolean sub_processing_on_thread_local=TRUE;

	GMutex* thread_loop_lock;

	space_id = header->space_id;
	kp_id = header->kp_id;
	tr_id = header->tr_id;

	s = g_new0(scheduler_item, 1);
	op_lock = g_mutex_new();
	op_cond = g_cond_new();

    thread_loop_lock =  g_mutex_new();

	s->header = header;
	s->req = req_msg;
	s->rsp = rsp_msg;
	s->op_lock = op_lock;
	s->op_cond = op_cond;
	s->op_complete = FALSE;

	s->thread_loop_lock=thread_loop_lock;

	s->thread_is_free=g_cond_new();
	s->thread_is_free_lock=g_mutex_new();

	s->rsp->param= param;

	//Time utilities
    struct timeval start, end;
    unsigned long mtime, seconds, useconds, global_t =0;

    int number_of_items;

	//rsp_msg->sparql_query_str=g_strdup(req_msg->query_str);

	g_async_queue_push(param->sib->query_queue, s);

	/* Signal scheduler that new operation has been added to queue */
	g_mutex_lock(param->sib->new_reqs_lock);
	param->sib->new_reqs = TRUE;
	// whiteboard_log_debug("Now signaling SCHEDULER in SUBSCRIBE tr %d\n", header->tr_id);
	g_cond_signal(param->sib->new_reqs_cond);
	g_mutex_unlock(param->sib->new_reqs_lock);

	/* Block while operation is being processed */
	g_mutex_lock(s->op_lock);
	while (!(s->op_complete))
	{
		g_cond_wait(s->op_cond, s->op_lock);
	}
	s->op_complete = FALSE;
	g_mutex_unlock(s->op_lock);


    if (param->sib->debug_latency)
    {	//TIME MEASUREMENT UTILITY LINES
		gettimeofday(&start, NULL);
		/////////////////////////////
    }

	//////////////////////////////////////
	whiteboard_log_debug("SPARQL first execution on the created context\n");
	rsp_msg->results_str=load_sparql_on_dedicate_sub_ts(s->rsp->sparql_query_str,s->rsp->subworld,s->rsp->substorage, s->rsp->submodel);
	//////////////////////////////////////

	whiteboard_util_send_method_return(param->conn,
			param->msg,
			DBUS_TYPE_STRING, &(space_id),
			DBUS_TYPE_STRING, &(kp_id),
			DBUS_TYPE_INT32, &(tr_id),
			DBUS_TYPE_INT32, &(rsp_msg->status),
			DBUS_TYPE_STRING, &(rsp_msg->sub_id),
			DBUS_TYPE_STRING, &(rsp_msg->results_str),
			WHITEBOARD_UTIL_LIST_END);

	dbus_message_unref(param->msg);
	g_free(rsp_msg->results_str);

    if (param->sib->debug_latency)
    {
		///////////////////////////////////////
		gettimeofday(&end, NULL);
		seconds  = end.tv_sec  - start.tv_sec;
		useconds = end.tv_usec - start.tv_usec;
		fprintf(stderr,"Thread First Sparql Time [us,sub_id] : %ld , %s \n",  seconds * 1000000 + useconds, rsp_msg->sub_id);
		///////////////////////////////////////
    }

	do
	{
	    //whiteboard_log_debug("Loop sub thread %s	\n", sub_id);

	    /* Check if unsubscribed */
	    g_mutex_lock(param->sib->subscriptions_lock);
	    sub_state = (subscription_state*)g_hash_table_lookup(param->sib->subs, sub_id);

	    if (sub_state->status == M3_SUB_STOPPED)
	    {

	        g_free(sub_state->sub_id);
	        g_free(sub_state->kp_id);

	        g_hash_table_remove(param->sib->subs, sub_id);
	        sub_state->unsub = TRUE;

	        g_cond_signal(sub_state->unsub_cond);
	        /* UNLOCK SUBSCRIPTION TABLE */
	        g_mutex_unlock(param->sib->subscriptions_lock);
	        /* UNLOCK UNSUB LOCK */

	        g_free(rsp_msg->sparql_query_str);

	        g_mutex_free(thread_loop_lock);
	    	g_cond_free(op_cond);
	    	g_mutex_free(op_lock);
	    	g_free(s);

	        break;
	    }

	    sub_state = (subscription_state*)g_hash_table_lookup(param->sib->subs, sub_id);

	    if (sub_state->status == M3_SUB_PENDING)
	    {
		    g_mutex_unlock(param->sib->subscriptions_lock);

	        g_mutex_lock(param->sib->new_reqs_lock);
	        param->sib->new_reqs = TRUE;
	        g_cond_signal(param->sib->new_reqs_cond);
	        g_mutex_unlock(param->sib->new_reqs_lock);
	        //whiteboard_log_debug("Subscription %s pending, setting new_reqs flag\n", rsp_msg->sub_id); /* SUB_DEBUG */
	    }
	    else
	    {
		    g_mutex_unlock(param->sib->subscriptions_lock);
	    }


	    /* Block while operation is processed */
	    g_mutex_lock(s->thread_loop_lock);
	    s->thread_busy=FALSE;
	    g_mutex_unlock(s->thread_loop_lock);

        //g_cond_signal(s->thread_is_free);

	    g_mutex_lock(s->op_lock);
	    while (!(s->op_complete))
	    {
	        g_cond_wait(s->op_cond, s->op_lock);
	    }
	    s->op_complete = FALSE;
	    g_mutex_unlock(s->op_lock);

	    if (param->sib->debug_latency)
	    {
			//TIME MEASUREMENT UTILITY LINES
			////////////////////////
			gettimeofday(&start, NULL);
			/////////////////////////////
	    }

	    g_mutex_lock(s->thread_loop_lock);
	    s->thread_busy=TRUE;
	    g_mutex_unlock(s->thread_loop_lock);

	    whiteboard_log_debug("Got new subscription result for transaction %d\n", tr_id);
	    //whiteboard_log_debug("Got new query result for subscription %s\n", rsp_msg->sub_id); /* SUB_DEBUG */

	    g_mutex_lock(rsp_msg->sub_processing_on_thread_lock);
	    sub_processing_on_thread_local=rsp_msg->sub_processing_on_thread;
	    g_mutex_unlock(rsp_msg->sub_processing_on_thread_lock);

	    if (sub_processing_on_thread_local)
	    {

	        ////////////////////
			g_mutex_lock(param->sib->sub_threads_lock);
			//printf("full active th: %d ", param->sib->sub_threads_active);
			if (param->sib->sub_threads_active  <  param->sib->sub_threads_max)
			    param->sib->sub_threads_active ++;
			g_mutex_unlock(param->sib->sub_threads_lock);
			////////////////////

			gboolean result;
			result = load_sparql_booster_subscriptions(s, param->sib);
			//printf ("Loaded SPARQL on thread, results found : %d\n", result);

            ////////////////////
            g_mutex_lock(param->sib->sub_threads_lock);
            if (param->sib->sub_threads_active  > 0)
                param->sib->sub_threads_active --;
            g_mutex_unlock(param->sib->sub_threads_lock);
            ////////////////////

	    }	//END FULL THREAD CASE

	    if (param->sib->debug_latency)
	    {
			///////////////////////////////////////
			gettimeofday(&end, NULL);
			seconds  = end.tv_sec  - start.tv_sec;
			useconds = end.tv_usec - start.tv_usec;
			global_t = seconds * 1000000 + useconds;
			fprintf(stderr,"Thread Sparql time [us,sub_id] : %d , %s \n",  global_t , rsp_msg->sub_id);
			///////////////////////////////////////
	    }
	}
	while(TRUE);

	return ss_StatusOK;
}

void destroy_recycle_world_sub_ts(ssap_sib_message* rsp, sib_data_structure* p)
{

	//Incredibly faster than destroying model-...
    /*
	librdf_stream * stream;
	stream= librdf_model_as_stream(rsp->submodel);
	while(!librdf_stream_end(stream))
	{
		librdf_model_remove_statement(rsp->submodel, librdf_stream_get_object(stream));
		librdf_stream_next(stream);
	}
	librdf_free_stream(stream);
    */

    librdf_free_model(rsp->submodel);
    librdf_free_storage(rsp->substorage);

    // UNRECYCLING STRATEGY
    //librdf_free_world(rsp->subworld);

    // RECYCLING WORLD STRATEGY

    //rsp->submodel=NULL;
    //rsp->substorage=NULL;
    //rsp->subworld=NULL;

	g_mutex_lock(p->sub_threads_lock);
	int sub_threads_max_local;
	sub_threads_max_local=p->sub_threads_max;
	g_mutex_unlock(p->sub_threads_lock);

	if (sub_threads_max_local != 0)
	{
		g_async_queue_lock(p->used_worlds_async_queue);
		g_async_queue_push_unlocked(p->used_worlds_async_queue, rsp->subworld);
		g_async_queue_unlock(p->used_worlds_async_queue);
	}
    // RECYCLING WORLD, MORE ECOLOGIC

}

void generate_recycle_world_sub_ts(ssap_sib_message* rsp, sib_data_structure* p)
{

	g_mutex_lock(p->sub_threads_lock);
	int sub_threads_max_local;
	sub_threads_max_local=p->sub_threads_max;
	g_mutex_unlock(p->sub_threads_lock);

	if (sub_threads_max_local == 0)
	{
	    rsp->subworld=p->RDF_world;
	}
	else
	{
		// RECYCLING WORLD, MORE ECOLOGIC
		librdf_world * subworld = NULL;

		g_async_queue_lock(p->used_worlds_async_queue);
		subworld = g_async_queue_try_pop_unlocked(p->used_worlds_async_queue);
		g_async_queue_unlock(p->used_worlds_async_queue);

		if (subworld==NULL)
		{
			//printf("NON Recycling world...\n");

			rsp->subworld = librdf_new_world();
			librdf_world_open(rsp->subworld );

			//int * librdf_log_func ;
			rasqal_world_set_warning_level(librdf_world_get_rasqal(rsp->subworld), 0);
		}
		else
		{
			//printf("Recycling world...\n");
			rsp->subworld =subworld;
		}
		// RECYCLING WORLDs, MORE ECOLOGIC
	}
    //rsp->subworld = librdf_new_world();
    //librdf_world_open(rsp->subworld );

    //////////////////////////////
    librdf_storage * substorage;
    librdf_model * submodel;

    if (!p->subs_persistent)
    {
        if (p->subs_hash)
               substorage=librdf_new_storage(rsp->subworld, "hashes", NULL, "hash-type='memory'");
        else
               substorage=librdf_new_storage(rsp->subworld,  NULL, NULL, NULL);
    }
    else
    {
        gchar** split_result_array;
        gchar** pointer_char_arr;
        gchar * tempname;

        split_result_array= g_strsplit(rsp->sub_id,subs_uri, 0);
        pointer_char_arr=split_result_array;
        pointer_char_arr++;
        tempname=g_strdup_printf("subts_%s",*pointer_char_arr);

        substorage=librdf_new_storage(rsp->subworld, "hashes", tempname,"new='yes',hash-type='bdb',dir='.'");

        g_free (tempname);
        g_strfreev (split_result_array);
    }

    //Initialize  Node with subscription name
    rsp->substorage=substorage;

    submodel=librdf_new_model(rsp->subworld, rsp->substorage, NULL);

    rsp->submodel=submodel;
}

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

		// TO DEFINE.. Commented because now just a waste of memory

		//g_mutex_lock(param->sib->members_lock);
		//kp_data = g_hash_table_lookup(param->sib->joined, kp_id);
		//if (kp_data != NULL)
		//{
		//  kp_data->subs = g_slist_prepend(kp_data->subs, g_strdup(temp_sub_id));
		//  kp_data->n_of_subs++;
		//}
		//g_mutex_unlock(param->sib->members_lock);

		/*
		 * Initialize the query structure and start
		   suitable subscription processor
		 */

		switch(req_msg->type)
		{
		case QueryTypeTemplate:
			status = parseM3_triples(&(req_msg->template_query), req_msg->query_str,   NULL);
			rsp_msg->sparql_subscribe=FALSE;

			if (status == ss_StatusOK)
			{
				/* ASSIGN SUB ID HERE *///////////////////////////////////////////
				temp_sub_id = g_strdup_printf("%s%s_%d",subs_uri, kp_id, tr_id);

				g_mutex_lock(param->sib->subscriptions_lock);
				while (g_hash_table_lookup(param->sib->subs, temp_sub_id)) {
					/* Create better sub_id generator! */
					g_free(temp_sub_id);
					temp_sub_id = g_strdup_printf("%s%s_%d", subs_uri,kp_id, ++tr_id);
				}

				sub_state->status = M3_SUB_ONGOING;
				//sub_state->sub_id = g_strdup(temp_sub_id);
				//sub_state->kp_id = g_strdup(kp_id);

				g_hash_table_insert(param->sib->subs, temp_sub_id, sub_state);
				g_mutex_unlock(param->sib->subscriptions_lock);

				rsp_msg->sub_id = g_strdup(temp_sub_id);
				rsp_msg->first_subscribe =TRUE;

				//////////////////////////////////////////////////////
				rsp_msg->insert_remove_queue_async = g_async_queue_new();
			    if (!rsp_msg->insert_remove_queue_async) exit(-1);

                ///////////////////////////////////////////////////////

			    rsp_msg->sub_processing_on_thread_lock=g_mutex_new();
			    if (!rsp_msg->sub_processing_on_thread_lock) exit(-1);

			    generate_recycle_world_sub_ts(rsp_msg, param->sib);

				//whiteboard_log_debug("Started triples subscription with id %s", rsp_msg->sub_id); /* SUB_DEBUG */
				//starting
				status = m3_subscribe_triples(header, req_msg, rsp_msg, param);
				//ended

				g_async_queue_unref (rsp_msg->insert_remove_queue_async);

				g_mutex_free(rsp_msg->sub_processing_on_thread_lock);

				g_mutex_lock(param->sib->subscriptions_lock);
				g_hash_table_remove(param->sib->subs,temp_sub_id);
				g_mutex_unlock(param->sib->subscriptions_lock);

				g_free(temp_sub_id);

				destroy_recycle_world_sub_ts(rsp_msg, param->sib);

			}
			else
			{

				gchar *nullstr = g_strdup_printf("");
				/////////////////////////////////

				gchar tmp[37];
				uuid_t u1;
				uuid_generate(u1);
				uuid_unparse(u1, tmp);

			   	rsp_msg->sub_id = g_strdup_printf("%ssytax_error_id_/%s",subs_uri, tmp);

				//whiteboard_log_debug("SPARQL subscribe parsing wrong\n");
				whiteboard_util_send_method_return(param->conn,
						param->msg,
						DBUS_TYPE_STRING, &(space_id),
						DBUS_TYPE_STRING, &(kp_id),
						DBUS_TYPE_INT32, &(tr_id),
						DBUS_TYPE_INT32, &(status),
						DBUS_TYPE_STRING, &(rsp_msg->sub_id),
						DBUS_TYPE_STRING, &(nullstr),
						WHITEBOARD_UTIL_LIST_END);

				g_free(nullstr);

			}
			break;


		case QueryTypeSPARQLSelect:
			g_mutex_lock  (param->sib->sparql_preprocessing_lock);
			status = parseM3_sparql_bindings(&(req_msg->template_query), &(rsp_msg->sparql_vars),&(rsp_msg->required_sparql_vars), req_msg->query_str, param->sib);
			g_mutex_unlock(param->sib->sparql_preprocessing_lock);

			//whiteboard_log_debug("RECEIVED SPARQL IN\n");
			//whiteboard_log_debug("q: %s\n",req_msg->query_str);

			rsp_msg->sparql_query_str=g_strdup(req_msg->query_str);
			rsp_msg->sparql_subscribe=TRUE;
			/////////////////////////////////////////

			if (status == ss_StatusOK)
			{
				/* ASSIGN SUB ID HERE */
				temp_sub_id = g_strdup_printf("%s%s_%d",subs_uri, kp_id, tr_id);

				g_mutex_lock(param->sib->subscriptions_lock);
				while (g_hash_table_lookup(param->sib->subs, temp_sub_id)) {
					/* Create better sub_id generator! */
					g_free(temp_sub_id);
					temp_sub_id = g_strdup_printf("%s%s_%d", subs_uri,kp_id, ++tr_id);
				}

				sub_state->status = M3_SUB_ONGOING;

				//sub_state->sub_id = g_strdup(temp_sub_id);
				//sub_state->kp_id = g_strdup(kp_id);

				g_hash_table_insert(param->sib->subs, temp_sub_id, sub_state);
				g_mutex_unlock(param->sib->subscriptions_lock);

				rsp_msg->sub_id = g_strdup(temp_sub_id);

				rsp_msg->first_subscribe =TRUE;

				rsp_msg->insert_remove_queue_async = g_async_queue_new();
			    if (!rsp_msg->insert_remove_queue_async) exit(-1);

			    rsp_msg->sub_processing_on_thread_lock=g_mutex_new();
			    if (!rsp_msg->sub_processing_on_thread_lock) exit(-1);

	            generate_recycle_world_sub_ts(rsp_msg, param->sib);

				//Starting thread loop
				status = m3_subscribe_sparql(header, req_msg, rsp_msg, param);
				//The thread loop is dead

				g_slist_free_full(rsp_msg->sparql_vars, g_free );
				g_slist_free_full(rsp_msg->required_sparql_vars, g_free );
				m3_additional_free_triple_sparql_list(&(req_msg->template_query));

                g_async_queue_unref (rsp_msg->insert_remove_queue_async);

				g_mutex_free(rsp_msg->sub_processing_on_thread_lock);

				g_mutex_lock(param->sib->subscriptions_lock);
				g_hash_table_remove(param->sib->subs,temp_sub_id);
				g_mutex_unlock(param->sib->subscriptions_lock);

				g_free(temp_sub_id);

				destroy_recycle_world_sub_ts(rsp_msg, param->sib);

			}
			else
			{
			    gchar *nullstr = g_strdup_printf("");
			    /////////////////////////////////

			    gchar tmp[37];
			    uuid_t u1;
			    uuid_generate(u1);
			    uuid_unparse(u1, tmp);


				rsp_msg->sub_id = g_strdup_printf("%ssytax_error_id_%s",subs_uri, tmp);
				//whiteboard_log_debug("SPARQL subscribe parsing wrong\n");

				whiteboard_util_send_method_return(param->conn,
						param->msg,
						DBUS_TYPE_STRING, &(space_id),
						DBUS_TYPE_STRING, &(kp_id),
						DBUS_TYPE_INT32, &(tr_id),
						DBUS_TYPE_INT32, &(status),
						DBUS_TYPE_STRING, &(rsp_msg->sub_id),
						DBUS_TYPE_STRING, &(nullstr),
						WHITEBOARD_UTIL_LIST_END);

				g_free(nullstr);

			}

			break;
		}
		//whiteboard_log_debug("SUBSCRIBE: subscription %s finished \n", rsp_msg->sub_id);
	}
	else
	{
		whiteboard_log_warning("Could not parse SUBSCRIBE method call message\n");
	}

	m3_free_triple_list_simple(&(req_msg->template_query));

	//g_free(req_msg->query_str);
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
			//g_free(sub);

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

            if ( param->virtuoso)
                librdf_model_context_add_statement(param->RDF_model, param->virtuoso_context, statement);
            else
                librdf_model_add_statement(param->RDF_model, statement);


			//FOR DIFFERENTIAL SUBSCRIBE
			//g_mutex_lock(param->temp_ins_rem_operations_lock);
			//librdf_model_add_statement(param->RDF_model_insert, statement);

            //printf("Inserting\n");
            //librdf_statement_print(statement , stdout);
            //printf("\n");

			statement_in_tl(&(param->RDF_list_insert), statement);

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

				/////////////////////Best way to solve blank nodes issues is not have them at all
                if (! blanks_founded)
                {
                    if ( param->virtuoso)
                        librdf_model_context_add_statement(param->RDF_model, param->virtuoso_context, statement);
                    else
                        librdf_model_add_statement(param->RDF_model, statement);

                    statement_in_tl(&(param->RDF_list_insert), statement);
                    reasoning(param,ttemp, param->enable_rdf_pp);
                }
				//////////////////////////////////////////////////////////////////////////////////

				ssFreeTriple(ttemp);

				librdf_stream_next(stream);
			}
			//g_mutex_unlock(param->temp_ins_rem_operations_lock);

			//fprintf(stderr, "RDF inserted ..\n");
			op->rsp->status = ss_StatusOK;
		}


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
				if (! tq->objType)
				{
					librdf_statement_set_object(partial_statement,
							librdf_new_node_from_uri_string(param->RDF_world,  tq->object));
				}
				else
				{
					librdf_statement_set_object(partial_statement,
							librdf_new_node_from_literal(param->RDF_world,tq->object, NULL, 0));
				}
			}

			param->subs_sheduler_removed_triple=TRUE;

			if (al_least_one_wildcard)
			{
				if (all_wildcard)
				{
					//If the TS is big I can't copy all the content in ram.. so I need this solution.
					whiteboard_log_debug("ALL WC! RESET DB\n");
					reset_storage_model( param);
				}
				else
				{
                    //One or more WC
                    if (param->virtuoso)
                        stream=librdf_model_find_statements_in_context(param->RDF_model, partial_statement,  param->virtuoso_context);
                    else
                        stream=librdf_model_find_statements(param->RDF_model, partial_statement);

                    while(!librdf_stream_end(stream))
                    {
                        statement=librdf_stream_get_object(stream);

                        //printf("Removing\n");
                        //librdf_statement_print(statement , stdout);
                        //printf("\n");

                        ////////////////
                        statement_in_tl(&(param->RDF_list_remove), statement);
                        ////////////////

                        if (param->virtuoso)
                        	librdf_model_context_remove_statement(param->RDF_model, param->virtuoso_context, statement);
                        else
                        	librdf_model_remove_statement(param->RDF_model, statement);

                        librdf_stream_next(stream);

                    }
                    librdf_free_stream(stream);
				}
			}
			else
			{
			    //No WC
			    ////////////////
			    statement_in_tl(&(param->RDF_list_remove), partial_statement);
                ///////////////

                //printf("Removing\n");
                //librdf_statement_print(partial_statement , stdout);
                //printf("\n");

                if (param->virtuoso)
                	librdf_model_context_remove_statement(param->RDF_model, param->virtuoso_context, partial_statement);
                else
                	librdf_model_remove_statement(param->RDF_model,partial_statement);

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

                if (param->virtuoso)
                    librdf_model_context_remove_statement(param->RDF_model, param->virtuoso_context, statement);
                else
                    librdf_model_remove_statement(param->RDF_model, statement);

                statement_in_tl(&(param->RDF_list_remove), statement);
                //librdf_model_add_statement(param->RDF_model_remove, statement);
                librdf_stream_next(stream);
			}
			//g_mutex_unlock(param->temp_ins_rem_operations_lock);


			fprintf(stderr, "RDF removed ..\n");
			op->rsp->status = ss_StatusOK;
		}


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

	/////////////////////////


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


			//whiteboard_log_debug("RDF reader: now querying for transaction %d\n", op->header->tr_id);
			//whiteboard_log_debug("RDF reader: triple is \n%s\n%s\n%s\n", tq->subject, tq->predicate, tq->object);


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
				if (! tq->objType)
				{	librdf_statement_set_object(partial_statement,
						librdf_new_node_from_uri_string(p->RDF_world, tq->object));
				}
				else
				{	librdf_statement_set_object(partial_statement,
						librdf_new_node_from_literal(p->RDF_world, tq->object, NULL, 0));
				}
			}

            if (p->virtuoso)
                stream=librdf_model_find_statements_in_context(p->RDF_model, partial_statement, p->virtuoso_context);
            else
                stream=librdf_model_find_statements(p->RDF_model, partial_statement);

			librdf_free_statement(partial_statement);


			//fprintf(stderr, "Query print all\n");
			while(!librdf_stream_end(stream)) {
				statement=librdf_stream_get_object(stream);

				//CHARGE FIRST TIME LOCAL TRIPLES IN LOCAL CONTEXT STORE///////////////////////////////////
				if (op->rsp->first_subscribe)
				{
					// NECESSARY FOR LITERAL^^STRING FROM VIRTUOSO
					if (p->virtuoso)
					{
						librdf_statement * forced_statement = librdf_new_statement_from_statement(statement);
						if (librdf_node_is_literal(librdf_statement_get_object(statement)))
						{
							librdf_free_node(librdf_statement_get_object(forced_statement));
							librdf_statement_set_object(forced_statement,
								librdf_new_node_from_literal(p->RDF_world,
									librdf_node_get_literal_value(librdf_statement_get_object(statement)), NULL, 0));
						}
						librdf_model_add_statement(op->rsp->submodel, forced_statement);
						librdf_free_statement(forced_statement);
					}
					else
					{
						librdf_model_add_statement(op->rsp->submodel, statement);
					}
				}
				///////////////////////////////////////////////////////////////////////////////////////////

				if (op->rsp->sparql_subscribe != TRUE)
				{
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

				}
				librdf_stream_next(stream);
			}

			librdf_free_stream(stream);
			//
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
	    op->rsp->status = ss_StatusOK;
		whiteboard_log_debug("RECEIVED SPARQL (query or sub ) IN\n");
		whiteboard_log_debug("q: %s\n",op->req->query_str);


		op->rsp->results_str=load_sparql_on_dedicate_sub_ts(op->req->query_str, p->RDF_world, p->RDF_storage, p->RDF_model);
		if (g_strcmp0(op->rsp->results_str,"")==0)
		{
		    op->rsp->status = ss_GeneralError;
		}
		break;
	}


	//End case query type
	}

	return op->rsp->status;
}


gboolean * load_seq_i_r_to_async_ts(scheduler_item* op, sib_data_structure* p)
{

	gboolean result =FALSE;

    gboolean  same_s;
    gboolean  same_p;
    gboolean  same_o;

    GSList * transaction_added_triples = NULL;
    GSList * transaction_removed_triples = NULL;

    added_removed_lists_struct * add_rm_transaction =g_new0 (added_removed_lists_struct,1);

    //Check if something has been added or removed..
    if (p->subs_sheduler_inserted_triple || p->subs_sheduler_removed_triple)
    {

        ssTriple_t_sparql* tq;
        GSList* query_list = op->req->template_query;

        /* Iterate over query here */
        while (query_list != NULL)
        {
            tq = query_list->data;
            if (!tq->subject || !tq->predicate || !tq->object)
            {
                op->rsp->status = ss_OperationFailed;
                op->rsp->results = NULL;
                break;
            }

            //////////////////FILTER OUT IDENTICAL QUERY PATTERNS//////////////////////
            if (op->rsp->sparql_subscribe)
            {
                if (tq->replicated)
                {
					query_list=query_list->next;
					continue;
                }
            }

            //////////////////END FILTER IDENTICAL QUERY PATTERNS//////////////////////
            if ((p->subs_sheduler_removed_triple ))
            {
                GSList * pointer_removed = p->RDF_list_remove;
                while (pointer_removed)
                {
                    ssTriple_t * removed = pointer_removed->data;

                    same_s=FALSE;
                    same_p=FALSE;
                    same_o=FALSE;

                    if (
                            (strcmp(removed->subject,tq->subject) ==0) ||
                            (strcmp(tq->subject,wildcard1) ==0) ||
                            (strcmp(tq->subject,wildcard2) ==0)
                    )
                    {
                        same_s=TRUE;
                    }
                    else
                    {
                        pointer_removed=pointer_removed->next;
                        continue;
                    }

                    if (
                            (strcmp(removed->predicate,tq->predicate) ==0) ||
                            (strcmp(tq->predicate,wildcard1) ==0) ||
                            (strcmp(tq->predicate,wildcard2) ==0)
                    )
                    {
                        same_p=TRUE;
                    }
                    else
                    {
                        pointer_removed=pointer_removed->next;
                        continue;
                    }

                    if (
                            ((strcmp(removed->object,tq->object) == 0) && ((removed->objType)==(tq->objType))) ||
                            (strcmp(tq->object,wildcard1) ==0) ||
                            (strcmp(tq->object,wildcard2) ==0)
                    )
                    {
                        same_o=TRUE;
                    }
                    else
                    {
                        pointer_removed=pointer_removed->next;
                        continue;
                    }

                    if (same_s && same_p && same_o)
                        //No wildcard on statement removed
                        {
                        ssTriple_t * copied_triple = ssTripleHardCopy(removed);

                        //printf("Removing triple from triplelist potentially removed \n");
                        //printf("%s %s %s %d\n",copied_triple->subject , copied_triple->predicate, copied_triple->object , copied_triple-> objType);

                        //g_async_queue_lock(op->rsp->insert_remove_queue_async);
                        transaction_removed_triples = g_slist_prepend(transaction_removed_triples,copied_triple);
                        //g_async_queue_unlock(op->rsp->insert_remove_queue_async);

                        result=TRUE;
                        }

                    pointer_removed=pointer_removed->next;
                }
            }

            if (p->subs_sheduler_inserted_triple )
            {
                GSList * pointer_added = p->RDF_list_insert;
                while (pointer_added)
                {
                    ssTriple_t * added = pointer_added->data;

                    same_s=FALSE;
                    same_p=FALSE;
                    same_o=FALSE;

                    if (
                            (strcmp(added->subject,tq->subject) ==0) ||
                            (strcmp(tq->subject,wildcard1) ==0) ||
                            (strcmp(tq->subject,wildcard2) ==0)
                    )
                    {
                        same_s=TRUE;
                    }
                    else
                    {
                        pointer_added=pointer_added->next;
                        continue;
                    }

                    if (
                            (strcmp(added->predicate,tq->predicate) ==0) ||
                            (strcmp(tq->predicate,wildcard1) ==0) ||
                            (strcmp(tq->predicate,wildcard2) ==0)
                    )
                    {
                        same_p=TRUE;
                    }
                    else
                    {
                        pointer_added=pointer_added->next;
                        continue;
                    }

                    if (
                            ((strcmp(added->object,tq->object) == 0) && ((added->objType)==(tq->objType))) ||
                            (strcmp(tq->object,wildcard1) ==0) ||
                            (strcmp(tq->object,wildcard2) ==0)
                    )
                    {
                        same_o=TRUE;
                    }
                    else
                    {
                        pointer_added=pointer_added->next;
                        continue;
                    }

                    if (same_s && same_p && same_o)
                        //No wildcard on statement removed
                        {

                        ssTriple_t * copied_triple = ssTripleHardCopy(added);

                        //printf("Adding triple from triplelist potentially added \n");
                        //printf("%s %s %s %d \n",copied_triple->subject ,  copied_triple->predicate, copied_triple->object , copied_triple-> objType);

                        //g_async_queue_lock(op->rsp->insert_remove_queue_async);
                        transaction_added_triples = g_slist_prepend(transaction_added_triples,copied_triple);
                        //g_async_queue_unlock(op->rsp->insert_remove_queue_async);

                        result=TRUE;
                        }

                    pointer_added=pointer_added->next;
                }

            }

            query_list = query_list->next;
        } //End while
    } // End something added or removed

    ///////////////////////////////////////

    if (result)
    {
		//g_async_queue_lock(op->rsp->insert_remove_queue_async);
		add_rm_transaction->removed_list= transaction_removed_triples;
		add_rm_transaction->added_list= transaction_added_triples;
		g_async_queue_push(op->rsp->insert_remove_queue_async, add_rm_transaction);
		//g_async_queue_push_unlocked(op->rsp->insert_remove_queue_async, add_rm_transaction);
		//g_async_queue_unlock(op->rsp->insert_remove_queue_async);
    }
    else
    {
        g_free(add_rm_transaction);
    }

    return (result);

}

//AVOIDABLE?
gboolean check_add_rm_on_substore_from_i_r(ssap_kp_message *req, ssap_sib_message* rsp )
{

    //printf ("check_add_rm_on_substore_from_i_r\n");
    gboolean sub_ts_has_changed = FALSE;
    gboolean queue_ended = FALSE;

    GSList * added_tlist = NULL;
    GSList * pointer_added_tlist = NULL;
    ssTriple_t * triple_added =NULL;

    GSList * removed_tlist = NULL;
    GSList * pointer_removed_tlist = NULL;
    ssTriple_t * triple_removed =NULL;

    librdf_statement * statement_added;
    librdf_statement * statement_removed;

    added_removed_lists_struct * add_rm_transaction = NULL;

    add_rm_transaction=g_async_queue_try_pop(rsp->insert_remove_queue_async);

    //g_async_queue_lock(rsp->insert_remove_queue_async);
    //add_rm_transaction=g_async_queue_try_pop_unlocked(rsp->insert_remove_queue_async);
    //g_async_queue_unlock(rsp->insert_remove_queue_async);

    if  (!add_rm_transaction )
    {    return FALSE;
    }
    else
    {   queue_ended=TRUE;
    }

    removed_tlist=add_rm_transaction->removed_list;
    added_tlist=add_rm_transaction->added_list;

    pointer_removed_tlist=removed_tlist;
    while (pointer_removed_tlist)
    {

        triple_removed= pointer_removed_tlist->data;

        ////////CHECKIG FOR IDENTICAL TRIPLES INSERTED AND REMOVED IN THE SAME UPDATE///
        gboolean found_in_added = false;
        pointer_added_tlist=add_rm_transaction->added_list;
        while (pointer_added_tlist)
        {
        	triple_added= pointer_added_tlist->data;
            if  (
                (g_strcmp0(triple_added->subject,triple_removed->subject)==0)&
                (g_strcmp0(triple_added->predicate,triple_removed->predicate)==0)&
                (g_strcmp0(triple_added->object,triple_removed->object)==0)&
                (triple_added->objType==triple_removed->objType)
                )
            {
            	found_in_added = true;
            	break;
            }
            pointer_added_tlist=pointer_added_tlist->next;
        }
        //////////////////////////////////////////////////////////////////////////////

        if (! found_in_added)
        {
			statement_removed=librdf_new_statement(rsp->subworld);
			librdf_statement_set_subject(statement_removed, librdf_new_node_from_uri_string(rsp->subworld,  triple_removed->subject));
			librdf_statement_set_predicate(statement_removed, librdf_new_node_from_uri_string(rsp->subworld,  triple_removed->predicate));

			if (! triple_removed->objType)
			{
				librdf_statement_set_object(statement_removed,
						librdf_new_node_from_uri_string(rsp->subworld, triple_removed->object));
			}
			else
			{
				librdf_statement_set_object(statement_removed,
						librdf_new_node_from_literal(rsp->subworld, triple_removed->object, NULL, 0));
			}


			librdf_stream * stream_tmp = NULL;
			stream_tmp =librdf_model_find_statements(rsp->submodel,statement_removed);

			//If the triple is present, it will be removed for sure.
			//REDLAND BUG; USING STREAM
			if (!librdf_stream_end(stream_tmp))
			{
				// really removed
				//sub_ts_has_changed=TRUE;

				//printf("Statement that will be really removed:\n");
				//librdf_statement_print(statement_removed, stdout);
				//printf("\n");

				//g_mutex_lock  (op->rsp->add_rm_lock);

				statement_in_tl(&(rsp->removed),statement_removed);
				//g_mutex_unlock  (op->rsp->add_rm_lock);
			}
			librdf_free_statement(statement_removed);
			librdf_free_stream(stream_tmp);

        }

        ssFreeTriple(triple_removed);
        pointer_removed_tlist=pointer_removed_tlist->next;
    }
    g_slist_free(removed_tlist);


    pointer_added_tlist=added_tlist;
    while (pointer_added_tlist)
    {
        triple_added= pointer_added_tlist->data;

		statement_added=librdf_new_statement(rsp->subworld);

		librdf_statement_set_subject(statement_added, librdf_new_node_from_uri_string(rsp->subworld,  triple_added->subject));
		librdf_statement_set_predicate(statement_added, librdf_new_node_from_uri_string(rsp->subworld,  triple_added->predicate));
		if (! triple_added->objType)
		{
			librdf_statement_set_object(statement_added,
					librdf_new_node_from_uri_string(rsp->subworld, triple_added->object));
		}
		else
		{
			librdf_statement_set_object(statement_added,
					librdf_new_node_from_literal(rsp->subworld, triple_added->object, NULL, 0));
		}

		//If the triple is not present, it will be added for sure.
		if (librdf_model_contains_statement(rsp->submodel,statement_added)==0)
		{
			// really removed
			//sub_ts_has_changed=TRUE;

			//printf("Statement that will be really added:\n");
			//librdf_statement_print(statement_added, stdout);
			//printf("\n");

			//g_mutex_lock  (op->rsp->add_rm_lock);
			statement_in_tl(&(rsp->added),statement_added);
			//g_mutex_unlock  (op->rsp->add_rm_lock);
		}

		librdf_free_statement(statement_added);

        ssFreeTriple (triple_added);
        pointer_added_tlist=pointer_added_tlist->next;
    }
    g_slist_free(added_tlist);


    g_free(add_rm_transaction);
    return queue_ended;

}

gboolean load_sparql_booster_subscriptions(scheduler_item* op, sib_data_structure* p)
{
    gchar *  added_str      =NULL;
    gchar *  removed_str    =NULL;
    gboolean value = FALSE;


	//Time utilities
    struct timeval start, end;
    unsigned long mtime, seconds, useconds, global_t =0;


    while (check_add_rm_on_substore_from_i_r(op->req, op->rsp ))
    {

    	//printf("SUBSTORAGE NOT UPDATED\n");
    	//librdf_model_print(op->rsp->submodel,stdout);


//        if (p->debug_latency)
//        {	//TIME MEASUREMENT UTILITY LINES
//    		gettimeofday(&start, NULL);
//    		/////////////////////////////
//        }
    	sparql_booster(op->req,op->rsp,1);
//        if (p->debug_latency)
//        {
//    		///////////////////////////////////////
//    		gettimeofday(&end, NULL);
//    		seconds  = end.tv_sec  - start.tv_sec;
//    		useconds = end.tv_usec - start.tv_usec;
//    		fprintf(stderr,"Booster Time Added [us,sub_id] : %ld , %s \n",  seconds * 1000000 + useconds, op->rsp->sub_id);
//    		///////////////////////////////////////
//        }


//        if (p->debug_latency)
//        {	//TIME MEASUREMENT UTILITY LINES
//    		gettimeofday(&start, NULL);
//    		/////////////////////////////
//        }
        remove_triplelist_in_submodel (p, op->rsp, op->rsp->removed);
        add_triplelist_in_submodel    (p, op->rsp, op->rsp->added);
//        if (p->debug_latency)
//        {
//    		///////////////////////////////////////
//    		gettimeofday(&end, NULL);
//    		seconds  = end.tv_sec  - start.tv_sec;
//    		useconds = end.tv_usec - start.tv_usec;
//    		fprintf(stderr,"Updating sub ts[us,sub_id] : %ld , %s \n",  seconds * 1000000 + useconds, op->rsp->sub_id);
//    		///////////////////////////////////////
//        }


//        if (p->debug_latency)
//        {	//TIME MEASUREMENT UTILITY LINES
//    		gettimeofday(&start, NULL);
//    		/////////////////////////////
//        }
        sparql_booster(op->req,op->rsp,0);
//        if (p->debug_latency)
//        {
//    		///////////////////////////////////////
//    		gettimeofday(&end, NULL);
//    		seconds  = end.tv_sec  - start.tv_sec;
//    		useconds = end.tv_usec - start.tv_usec;
//    		fprintf(stderr,"Booster Time Removed [us,sub_id] : %ld , %s \n",  seconds * 1000000 + useconds, op->rsp->sub_id);
//    		///////////////////////////////////////
//        }



        if ((op->rsp->added_booster == NULL) && (op->rsp->removed_booster ==NULL))
        {
            m3_free_triple_list_simple(&(op->rsp->added));
            m3_free_triple_list_simple(&(op->rsp->removed));

            value= value | FALSE;
            continue;
        }

        value= value | TRUE;

        added_str   = turn_sparql_booster_results_to_str(op->rsp->added_booster  ,  op->rsp->required_sparql_vars);
        removed_str = turn_sparql_booster_results_to_str(op->rsp->removed_booster,  op->rsp->required_sparql_vars);

        if( ++(op->rsp->ind_seqnum) == SSAP_IND_WRAP_NUM )
            op->rsp->ind_seqnum=1;


        sib_op_parameter * sib_op_param = op->rsp->param;

        whiteboard_util_send_signal(SIB_DBUS_OBJECT,
                SIB_DBUS_KP_INTERFACE,
                SIB_DBUS_KP_SIGNAL_SUBSCRIPTION_IND,
                sib_op_param->conn,
                DBUS_TYPE_STRING, &(op->header->space_id),
                DBUS_TYPE_STRING, &(op->header->kp_id),
                DBUS_TYPE_INT32, &(op->header->tr_id),
                DBUS_TYPE_INT32, &(op->rsp->ind_seqnum),
                DBUS_TYPE_STRING, &(op->rsp->sub_id),
                DBUS_TYPE_STRING, &(added_str),
                DBUS_TYPE_STRING, &(removed_str),
                WHITEBOARD_UTIL_LIST_END);

        m3_free_triple_list_simple(&(op->rsp->added));
        m3_free_triple_list_simple(&(op->rsp->removed));

        g_free(added_str);
        g_free(removed_str);

        clean_sparql_booster_results(&(op->rsp->added_booster));
        clean_sparql_booster_results(&(op->rsp->removed_booster));

    }

    return value;

}

gboolean load_rdf_subscriptions(scheduler_item* op, sib_data_structure* p)
{
    gchar *  added_str      =NULL;
    gchar *  removed_str    =NULL;
    gboolean value = FALSE;

    while (check_add_rm_on_substore_from_i_r(op->req, op->rsp ))
    {
        //printf("Loading rdf sub_seq\n");

        if ((op->rsp->added == NULL) && (op->rsp->removed == NULL))
        {
            value= value | FALSE;
            continue;
        }

        value= value | TRUE;

        add_triplelist_in_submodel    (p, op->rsp, op->rsp->added);
        remove_triplelist_in_submodel (p, op->rsp, op->rsp->removed);

        added_str=m3_gen_triple_string(op->rsp->added);
        removed_str=m3_gen_triple_string(op->rsp->removed);

        if( ++(op->rsp->ind_seqnum) == SSAP_IND_WRAP_NUM )
            op->rsp->ind_seqnum=1;


        sib_op_parameter * sib_op_param = op->rsp->param;

        whiteboard_util_send_signal(SIB_DBUS_OBJECT,
                SIB_DBUS_KP_INTERFACE,
                SIB_DBUS_KP_SIGNAL_SUBSCRIPTION_IND,
                sib_op_param->conn,
                DBUS_TYPE_STRING, &(op->header->space_id),
                DBUS_TYPE_STRING, &(op->header->kp_id),
                DBUS_TYPE_INT32, &(op->header->tr_id),
                DBUS_TYPE_INT32, &(op->rsp->ind_seqnum),
                DBUS_TYPE_STRING, &(op->rsp->sub_id),
                DBUS_TYPE_STRING, &(added_str),
                DBUS_TYPE_STRING, &(removed_str),
                WHITEBOARD_UTIL_LIST_END);


        g_free(added_str);
        g_free(removed_str);

        m3_free_triple_list_simple(&(op->rsp->added));
        m3_free_triple_list_simple(&(op->rsp->removed));

    }

    return value;

}

gboolean threadsub_wake_if_unsub(scheduler_item* op, sib_data_structure* p)
{
	//CHECK IF UNSUBSCRIBED!
	subscription_state* s;
	g_mutex_lock(p->subscriptions_lock);
	s = g_hash_table_lookup(p->subs, op->rsp->sub_id);
	if ((s->status == M3_SUB_STOPPED) || (s==NULL))
	{
		g_mutex_unlock(p->subscriptions_lock);

        /////////////////////////////
        op->op_complete = TRUE;
        g_cond_signal(op->op_cond);
        /////////////////////////////

        return TRUE;
	}
	else
	{
	    g_mutex_unlock(p->subscriptions_lock);
	    return FALSE;
	}
}

void threadsub_wake_and_reload_op(scheduler_item* op, sib_data_structure* p)
{
    //printf("Try Waking the Thread \n");

    //Trigger for unsub
    subscription_state* s;
    g_mutex_lock(p->subscriptions_lock);
    s = g_hash_table_lookup(p->subs, op->rsp->sub_id);
    if (NULL != s && s->status != M3_SUB_STOPPED)
    {
        s->status = M3_SUB_ONGOING;
        g_mutex_unlock(p->subscriptions_lock);
        //whiteboard_log_debug("QUERY FOR SUBS: Set subscription %s to ongoing\n", s->sub_id); /* SUB_DEBUG */

        g_async_queue_push(p->query_queue, op);
    }
    else if ((s->status == M3_SUB_STOPPED) || (NULL == s))
    {
        g_mutex_unlock(p->subscriptions_lock);
    }
    //If the thread is busy no signal is required
    g_mutex_lock(op->thread_loop_lock);
    if (op->thread_busy==FALSE)
    {
		/////////////////////////////
		op->op_complete = TRUE;
		g_cond_signal(op->op_cond);
		/////////////////////////////
    }
    g_mutex_unlock(op->thread_loop_lock);
}

void threadsub_try_not_wake_reload_op(scheduler_item* op, sib_data_structure* p)
{
    //Not necessary to wake the thread, just in case of unsubs
    //If no unsub happens the op will be autorecharged

    ///////////////////////////////////////////////////////////
    //Trigger for unsub

    subscription_state* s;
    g_mutex_lock(p->subscriptions_lock);
    s = g_hash_table_lookup(p->subs, op->rsp->sub_id);
    if ((NULL != s && s->status == M3_SUB_STOPPED) || (NULL == s))
    {
        g_mutex_unlock(p->subscriptions_lock);

        ////////////////////////////////////////
        //printf("cleaning submodels\n");
        //If the thread is busy no signal is required
        g_mutex_lock(op->thread_loop_lock);
        if (op->thread_busy==FALSE)
        {
        	//printf("seq to thread:UNSUB\n");

        	/////////////////////////////
			op->op_complete = TRUE;
			g_cond_signal(op->op_cond);
			/////////////////////////////

        }
        g_mutex_unlock(op->thread_loop_lock);

    }
    else //SUB still active, the persistent query will be auto-recharged
    {
        g_mutex_unlock(p->subscriptions_lock);
        g_async_queue_push(p->query_queue, op);
    }
    ////////////////////////////////////////////////////////


}

//Persistent query emulator.
//It emulate persistent query checking sheduled insert/remove
//operations in every sheduled cycle

void rdf_subscribe(scheduler_item* op, sib_data_structure* p)
{
	gboolean sub_ts_could_change=FALSE;
	gboolean sub_ts_will_change=FALSE;
	gboolean try_wake_thread=FALSE;
	gboolean sub_processing_on_thread_localvar;

	if (threadsub_wake_if_unsub(op,p))
	{
	    //printf("UNSUBSCRIBE ARRIVED, THREADS WAKED! \n");
	    return;
	}

	//CHECK IF A THREAD IS AVAILABLE

	g_mutex_lock(p->sub_threads_lock);
	int sub_threads_active_local;
	int sub_threads_max_local;
	sub_threads_active_local=p->sub_threads_active;
	sub_threads_max_local=p->sub_threads_max;
	g_mutex_unlock(p->sub_threads_lock);


	if (sub_threads_active_local  < sub_threads_max_local)
	{
		//printf("p->sub_threads_active %d on %d\n",sub_threads_active_local ,sub_threads_max_local );

		g_mutex_lock(op->rsp->sub_processing_on_thread_lock);
		op->rsp->sub_processing_on_thread=TRUE;
		sub_processing_on_thread_localvar=TRUE;
		g_mutex_unlock(op->rsp->sub_processing_on_thread_lock);
	}
	else
	{
		gboolean thread_busy_local;
        g_mutex_lock(op->thread_loop_lock);
        thread_busy_local=op->thread_busy;
        g_mutex_unlock(op->thread_loop_lock);

        while (thread_busy_local == TRUE)
        {
            //Not so elegant for now, but it works fine
            usleep(250);

            g_mutex_lock(op->thread_loop_lock);
            thread_busy_local=op->thread_busy;
            g_mutex_unlock(op->thread_loop_lock);
        }

        g_mutex_lock(op->rsp->sub_processing_on_thread_lock);
        op->rsp->sub_processing_on_thread=FALSE;
        sub_processing_on_thread_localvar=FALSE;
        g_mutex_unlock(op->rsp->sub_processing_on_thread_lock);

	}

	sub_ts_could_change = load_seq_i_r_to_async_ts(op, p);
	if (sub_ts_could_change)
    {
	    if (! sub_processing_on_thread_localvar)
	    {
	        if (op->rsp->sparql_subscribe)	//SPARQL SUBSCRIBE
	        {	try_wake_thread=load_sparql_booster_subscriptions(op,p);
	        }
	        else							//RDF SUBSCRIBE
	        {   try_wake_thread=load_rdf_subscriptions(op,p);
	        }

	        threadsub_try_not_wake_reload_op(op, p);
	    }
	    else
	    {	threadsub_wake_and_reload_op(op, p);
	    }
	}
	else // the ins/rm triples will not change sub-TS (in both cases RDF / SPARQL), check for unsubs
	    threadsub_try_not_wake_reload_op(op, p);

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

	GCond* new_reqs_cond = p->new_reqs_cond;
	GMutex* new_reqs_lock = p->new_reqs_lock;

	GHashTable* subs = p->subs;
	GMutex* subscriptions_lock = p->subscriptions_lock;

	gboolean updated = false;

	GSList* i_list = NULL;
	GSList* q_s_list = NULL;

	scheduler_item* op;

	//time managment
	struct timeval start, end;
	unsigned long mtime, seconds, useconds;


	while (TRUE)
	{

		/* Wait until there is actually something in the queues */
		g_mutex_lock(new_reqs_lock);
		while (!(p->new_reqs))
		{
			g_cond_wait(new_reqs_cond, new_reqs_lock);
		}
		p->new_reqs = FALSE;
		g_mutex_unlock(new_reqs_lock);

		//Freeze scheduler to perform sparql updates
		g_mutex_lock(p->sparql_update_lock);

		whiteboard_log_debug("Schedule Cycle Begin\n");


		/* Lock insert and query queues
        Insert contents into lists for processing
		*/

		g_async_queue_lock(i_queue);
		while (NULL !=
				(op = (scheduler_item*)g_async_queue_try_pop_unlocked(i_queue)))
		{

			if (p->enable_protection)
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
			q_s_list = g_slist_prepend(q_s_list, op);
			whiteboard_log_debug("Added item to query list");
		}
		g_async_queue_unlock(q_queue);


		//Clean Redland Temp Ins/Rem Triplestores
		//g_mutex_lock(p->temp_ins_rem_operations_lock);
		m3_free_triple_list_simple(&(p->RDF_list_insert));
		m3_free_triple_list_simple(&(p->RDF_list_remove));
		//g_mutex_unlock(p->temp_ins_rem_operations_lock);

		p->subs_sheduler_inserted_triple= FALSE;
		p->subs_sheduler_removed_triple=  FALSE;


		if (p->debug_latency && i_list)
		{
			//TIME MEASUREMENT UTILITY LINES
			//////////////////////////
			gettimeofday(&start, NULL);
			/////////////////////////////
		}

		g_slist_foreach(i_list, do_insert, p);
		///////////////////////////////////////
		if (p->debug_latency && i_list)
		{
			gettimeofday(&end, NULL);
			seconds  = end.tv_sec  - start.tv_sec;
			useconds = end.tv_usec - start.tv_usec;
			fprintf(stderr,"Scheduler Insert T [us] :%d \n", (seconds*1000000 + useconds));
		}
		///////////////////////////////////////
		g_slist_free(i_list);
		i_list = NULL;



		if (updated)
		{
			g_mutex_lock(subscriptions_lock);
			g_hash_table_foreach(subs, set_sub_to_pending, NULL);
			g_mutex_unlock(subscriptions_lock);

			updated = false;
		}

		if (p->debug_latency && q_s_list)
		{
			//TIME MEASUREMENT UTILITY LINES
			//////////////////////////
			gettimeofday(&start, NULL);
			/////////////////////////////
		}
		//whiteboard_log_debug("RDF store updated, set all subscriptions to pending\n"); /* SUB_DEBUG */
		g_slist_foreach(q_s_list, do_query_subscribe, p);
		///////////////////////////////////////
		if (p->debug_latency && q_s_list)
		{
			gettimeofday(&end, NULL);
			seconds  = end.tv_sec  - start.tv_sec;
			useconds = end.tv_usec - start.tv_usec;
			fprintf(stderr,"Scheduler Query T [us] : %d \n", (seconds*1000000 + useconds));
		}
		///////////////////////////////////////
		g_slist_free(q_s_list);
		q_s_list = NULL;

		//End freeze scheduler to perform sparql updates
		g_mutex_unlock(p->sparql_update_lock);

	}

}

/*
 * Initialization of SIB internal data structures
 * Contains
 * - mutex(s) for KP membership, subscription state and
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

    sd->used_worlds_async_queue= g_async_queue_new();
    if (NULL == sd->used_worlds_async_queue) exit(-1);

    sd->joined = g_hash_table_new(g_str_hash, g_str_equal);
    if (NULL == sd->joined) exit(-1);

    sd->subs = g_hash_table_new(g_str_hash, g_str_equal);
    if (NULL == sd->subs) exit(-1);

    //sd->members_lock = g_mutex_new();
    //if (NULL == sd->members_lock) exit(-1);

    sd->subscriptions_lock = g_mutex_new();
    if (NULL == sd->subscriptions_lock) exit(-1);

    sd->new_reqs_lock = g_mutex_new();
    if (NULL == sd->new_reqs_lock) exit(-1);

    sd->new_reqs_cond = g_cond_new();
    if (NULL == sd->new_reqs_cond) exit(-1);

    sd->dbus_indication_lock=g_mutex_new();
    if (!sd->dbus_indication_lock) exit(-1);

    sd->sub_threads_lock=g_mutex_new();
    if (NULL == sd->sub_threads_lock) exit(-1);

    sd->sparql_update_lock=g_mutex_new();
    if (NULL == sd->sparql_update_lock) exit(-1);

    sd->new_reqs = FALSE;

    //INITIALIZE REDLAND ENVIROMENT
    sd->RDF_world=librdf_new_world();
    librdf_world_open(sd->RDF_world);
    rasqal_world_set_warning_level(librdf_world_get_rasqal(sd->RDF_world), 0);

    //CHECK FOR PARAMETERS
    sd->enable_rdf_pp   =   FALSE;
    sd->enable_protection =    FALSE;
    sd->mem_volatile      =     FALSE;
    sd->sqlite =        FALSE;
    sd->virtuoso =  FALSE;
    sd->subs_persistent   =  FALSE;

    sd->debug_latency = FALSE;

    //////////INITIALIZED TO SEQUENTIAL SUBSCRIBE////////
    sd->sub_threads_max=INT_MAX;
    sd->sub_threads_active=0;
    /////////////////////////////////////////////////////

    for ( ; param_list != NULL ; param_list = g_slist_next(param_list))
    {
        gchar* current_parameter= param_list->data;
        if (strcmp ((char*)"--enable-protections",(char*)current_parameter)==0)
        {
            printf  ("Launching with write protections option\n");
            sd->enable_protection = FALSE;
        }
        if ((strcmp ((char*)"--storage-volatile",(char*)current_parameter)==0) || (strcmp ((char*)"--ram",(char*)current_parameter)==0))
        {
            printf  ("Launching with no persistent DB\n");
            sd->mem_volatile = TRUE;
            sd->mem_volatile_hash = FALSE;
        } else if
            ((strcmp ((char*)"--storage-volatile-hash",(char*)current_parameter)==0) || (strcmp ((char*)"--ram-hash",(char*)current_parameter)==0))
        {
            printf  ("Launching with non persistent DB using Hash\n");
            sd->mem_volatile = FALSE;
            sd->mem_volatile_hash = TRUE;
        }
        if (strcmp ((char*)"--storage-sqlite",(char*)current_parameter)==0)
        {
            printf  ("Launching with Sqlite storage (SLOW!)\n");
            sd->sqlite = TRUE;
        }
        if (strcmp ((char*)"--storage-virtuoso",(char*)current_parameter)==0)
        {

			printf  ("Loading Virtuoso, working on graph: <http://sib#%s> \n", sd->ss_name);
			sd->virtuoso = TRUE;

			printf  ( "\n");
			printf  ( "Setting parameters..      \n");
			char host [100] ;
			printf  ( "\n");
			printf               ("host (e.g. =localhost): ");
			scanf("%s", &host);
			char user [50];
			printf               ("user (e.g. =dba)     : ");
			scanf("%s", &user);
			char *pass = getpass("password             : ");
			printf  ("\n");
			sd->virtuoso_param=g_strdup_printf("dsn='VOS', host='%s', user='%s',password='%s'",host,user,pass);
			//printf  ("%s\n",sd->virtuoso_param);

        }
        if (strstr (current_parameter,(char*)"--storage-virtuoso-p" ) != NULL)
        {

			printf  ("Loading Virtuoso, working on graph: <http://sib#%s> \n", sd->ss_name);
			sd->virtuoso = TRUE;

            gchar** split_param_point =NULL;
            gchar** split_param =NULL;
            gchar * params;

            char * tempparam = strdup(current_parameter);
            split_param=g_strsplit(tempparam, "==" , 0);
            split_param_point=split_param;
            split_param++;

            params = string_substitution_(*split_param,"\"", "");

            sd->virtuoso_param=g_strdup(params);

        }
        if (strcmp ((char*)"--subs-persistent",(char*)current_parameter)==0)
        {
            printf  ("Launching with BDB persistent subscriptions triplestores\n");
            sd->subs_persistent = TRUE;
            sd->subs_hash=FALSE;
        }
        else if (strcmp ((char*)"--subs-hash",(char*)current_parameter)==0)
        {
            printf  ("Launching with Hash volatile subscriptions triplestores\n");
            sd->subs_persistent = FALSE;
            sd->subs_hash=TRUE;
        }

        if (strcmp ((char*)"--enable-rdf++",(char*)current_parameter)==0)
        {
            printf  ("Launching Reasoner. No persistent DB will be rescued !\n");
            sd->enable_rdf_pp = TRUE;
        }
        if (strstr (current_parameter,(char*)"--threads" ) != NULL)
        {

            gchar** split_param_point =NULL;
            gchar** split_param =NULL;
            char * tempparam = strdup(current_parameter);
            split_param=g_strsplit(tempparam, "=" , 0);
            split_param_point=split_param;
            split_param++;

            if (g_strcmp0(*split_param,"inf")==0)
            {
                sd->sub_threads_max=INT_MAX;
                printf  ("Setting number of sparql booster threads to unlimited\n", sd->sub_threads_max);
            }
            else
            {
                sd->sub_threads_max=atoi(*split_param);
                if (sd->sub_threads_max == NULL)
                {
                    printf ( "Setting no sparql booster threads\n\n");
                }
                else
                    printf ( "Setting number of sparql booster threads to : %d\n", sd->sub_threads_max);
            }
            g_strfreev (split_param_point);

        }

        if (strstr (current_parameter,(char*)"--debug-latency" ) != NULL)
        {
        	printf ( "Setting debug latency\n\n");
        	sd->debug_latency=TRUE;
        }

    }

    //////Creating Model and storage ////////////////////////
    initialize_storage_model(sd);
    ////////////////////////////////////////////////////////

    //ARCES DIFFERENTIAL SUBS INS/REM SCHEDULER TRIPLESTORES//////////////////////////////////////////////////////////
    sd->RDF_list_insert=NULL;
    sd->RDF_list_remove=NULL;

    sd->temp_ins_rem_operations_lock = g_mutex_new();

    //sparql subscribe modules
    sd->sparql_preprocessing_world = rasqal_new_world();
    rasqal_world_set_warning_level(sd->sparql_preprocessing_world, 0);
    sd->sparql_preprocessing_lock = g_mutex_new();



    ////REASONING INIT
    sd->subClasses =    g_hash_table_new_full(g_str_hash, g_str_equal,  g_free, NULL);
    sd->subProperties =     g_hash_table_new_full(g_str_hash, g_str_equal,  g_free, NULL);
    sd->propertyDomain=     g_hash_table_new_full(g_str_hash, g_str_equal,  g_free, NULL);
    sd->propertyRange=  g_hash_table_new_full(g_str_hash, g_str_equal,  g_free, NULL);
    ////--REASONING

    /* Start scheduler */
    g_thread_create(scheduler, sd, FALSE, NULL);


    if (NULL == sd->new_reqs_cond) exit(-1);
    if (NULL == sd->RDF_model) exit(-1);

    return sd;
}


//TIME MEASUREMENT UTILITY LINES
//////////////////////////
//struct timeval start, end;
//long mtime, seconds, useconds;
//gettimeofday(&start, NULL);
/////////////////////////////

///////////////////////////////////////
//gettimeofday(&end, NULL);
//seconds  = end.tv_sec  - start.tv_sec;
//useconds = end.tv_usec - start.tv_usec;
//fprintf(stderr,"Elapsed time post-parsing string: %ld seconds, useconds %ld \n", seconds,useconds);
///////////////////////////////////////
