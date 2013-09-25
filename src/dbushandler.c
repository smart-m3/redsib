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

/*
 * SIB daemon dbus handler
 *
 * dbushandler.c
 *
 * Copyright 2007 Nokia Corporation
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <glib.h>
#include <stdlib.h>
#include <string.h>

#define DBUS_API_SUBJECT_TO_CHANGE

#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <dbus/dbus-glib-bindings.h>

#define WHITEBOARD_LOG_C

#include <sib_dbus_ifaces.h>
#include <whiteboard_util.h>
#include <whiteboard_log.h>
#include <sibdefs.h>

#include "dbushandler.h"
#include "sib_operations.h"

struct _DBusHandler
{
  GList *kp_connections;
  GList *control_connections;
  /* UUID -> dbus connection */
  GHashTable *connection_map;

  GMainLoop *loop;
  DBusConnection *session_bus;
  gchar *local_address;
  gchar *my_uri;
  sib_data_structure* sib_data;

  GMutex *lock;

  GThreadPool *threadpool;
};

typedef struct _rmData
{
  gpointer value;
  gpointer key;
} rmData;
  

/* Keep this preprocessor instruction always AFTER struct definitions
   and BEFORE any function declaration/prototype */
#ifndef UNIT_TEST_INCLUDE_IMPLEMENTATION

/* Private function prototypes */

static int dbushandler_initialize(DBusHandler* self);

static void dbushandler_handle_connection(DBusServer* server,
					  DBusConnection* conn,
					  gpointer data);

static DBusHandlerResult dbushandler_handle_message(DBusConnection* conn,
						    DBusMessage* msg,
						    gpointer data);

static DBusHandlerResult dbushandler_kp_message(DBusHandler* self,
						DBusConnection* conn,
						DBusMessage* msg);
  
static void dbushandler_unregister_handler(DBusConnection* conn,gpointer data);


static int dbushandler_register_control(DBusHandler *self, DBusConnection *conn,
					DBusMessage *msg);

static int dbushandler_register_kp(DBusHandler *self, DBusConnection *conn,
				     DBusMessage *msg);
static int dbushandler_unregister_kp(DBusHandler *self, DBusConnection *conn,
				     DBusMessage *msg);

static void dbushandler_handle_disconnect( DBusHandler *self, DBusConnection *conn);
static gboolean dbushandler_compare_hashtable_value(gpointer _key, gpointer _value, gpointer _data);

static gint dbushandler_send_register_sib(DBusHandler* self, DBusConnection* conn);
static void kp_handler(gpointer data, gpointer userdata);

/* Public functions */

/**
 * Creates new dbushandler instance
 *
 * @return pointer to dbushandler instance
 */
DBusHandler* dbushandler_new(gchar* local_address, gchar *uri, GMainLoop* loop, sib_data_structure* sib_data)
{
  static gboolean instantiated = FALSE;
  DBusHandler *self = NULL;
  GError *gerror=NULL;
  whiteboard_log_debug_fb();
  
  g_return_val_if_fail(NULL != local_address, NULL);
  g_return_val_if_fail(NULL != loop, NULL);
  
  if (instantiated == TRUE)
    {
      /* Can be instantiated only once */
      whiteboard_log_error("DBusHandler already instantiated. "		\
			   "Won't create another instance.\n");
      whiteboard_log_error("DBusHandler already instantiated. "		\
			   "Won't create another instance.\n");
      return NULL;
    }
  
  self = g_new0(struct _DBusHandler, 1);
  
  self->loop = loop;
  g_main_loop_ref(loop);
  
  self->local_address = g_strdup(local_address);
  self->my_uri = g_strdup(uri);
  self->kp_connections = NULL;
  
  self->connection_map = g_hash_table_new_full(g_str_hash, g_str_equal,
					       g_free, NULL);
  self->lock = g_mutex_new();
  self->sib_data = sib_data;

  self->threadpool = g_thread_pool_new( kp_handler, self, -1, FALSE, &gerror );
  if(gerror)
    {
      whiteboard_log_error("Could not create threadpool for kp_handler: %s\n", gerror->message);
      g_error_free(gerror);
      return NULL;
    }

  if (-1 == dbushandler_initialize(self))
    {
      whiteboard_log_error("DBusHandler initialization failed.\n");
    }
	
  if (NULL != self)
    instantiated = TRUE;

  whiteboard_log_debug_fe();

  return self;
}

void dbushandler_destroy(DBusHandler *self)
{
  whiteboard_log_debug_fb();

  g_return_if_fail(NULL != self);
  g_mutex_lock(self->lock);
  g_free(self->local_address);
  g_free(self->my_uri);
  g_hash_table_destroy(self->connection_map);

  g_list_free(self->kp_connections);
  g_mutex_unlock(self->lock);
  g_mutex_free(self->lock);
  g_main_loop_unref(self->loop);	
  g_free(self);

  whiteboard_log_debug_fe();
}

GList *dbushandler_get_control_connections(DBusHandler *self)
{
  g_return_val_if_fail(NULL != self, NULL);

  return self->control_connections;
}

GList *dbushandler_get_kp_connections(DBusHandler *self)
{
  g_return_val_if_fail(NULL != self, NULL);

  return self->kp_connections;
}

DBusConnection *dbushandler_get_session_bus(DBusHandler *self)
{
  g_return_val_if_fail(NULL != self, NULL);

  return self->session_bus;
}

/* Private functions */

static int dbushandler_register_kp(DBusHandler *self, DBusConnection *conn,
				    DBusMessage *msg)
{
  gchar* registered_uuid = NULL;
  gint status = -1;
  whiteboard_log_debug_fb();

  /* TODO: browse_id -> unique_id? -> util? */

  whiteboard_util_parse_message(msg, DBUS_TYPE_STRING, &registered_uuid,
			 DBUS_TYPE_INVALID);
  //  unique_name = g_strdup_printf(":%d", sib_sib_handler_get_access_id());
  //whiteboard_log_debug("Setting unique name for ui connection: %s\n", registered_uuid, unique_name);
  
  //  dbus_bus_set_unique_name(conn, unique_name); 
  //dbushandler_add_connection_by_uuid(self, g_strdup(unique_name), conn);
  
  dbushandler_add_connection_by_uuid(self, g_strdup(registered_uuid), conn);

  //  g_free(unique_name);

  self->kp_connections = g_list_prepend(self->kp_connections, conn);

  status = 0;
  whiteboard_util_send_method_return(conn, msg,
				     DBUS_TYPE_INT32, &status,
				     WHITEBOARD_UTIL_LIST_END);


  whiteboard_log_debug_fe();

  /* g_free(registered_uuid); */
  return DBUS_HANDLER_RESULT_HANDLED;
}

static int dbushandler_unregister_kp(DBusHandler *self, DBusConnection *conn,
				      DBusMessage *msg)
{
  gchar* uuid = NULL;
  
  whiteboard_log_debug_fb();
  
  
  whiteboard_util_parse_message(msg,
			 DBUS_TYPE_STRING, &uuid,
			 DBUS_TYPE_INVALID);
  
  
  if( dbushandler_remove_connection_by_uuid(self, uuid))
    {
      whiteboard_log_debugc(WHITEBOARD_DEBUG_DBUS, "Node connection w/ uuid: %s removed\n", uuid);
    }

  whiteboard_log_debug_fe();

  return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

static int dbushandler_register_control(DBusHandler *self, DBusConnection *conn,
					DBusMessage *msg)
{
  gchar *registered_uuid = NULL;
  int status = -1;
  whiteboard_log_debug_fb();
  //unique_name = g_strdup_printf(":%d", sib_sib_handler_get_access_id());
  //whiteboard_log_debug("Setting unique name for control connection: %s\n", unique_name);

  whiteboard_util_parse_message(msg, DBUS_TYPE_STRING, &registered_uuid,
				DBUS_TYPE_INVALID);

  whiteboard_log_debug("Registered uuid: %s\n", registered_uuid);

  dbushandler_add_connection_by_uuid(self, registered_uuid, conn);

  //dbus_bus_set_unique_name(conn, unique_name); 
  //g_free(unique_name);

  self->control_connections = g_list_prepend(self->control_connections,
					     conn);


  status = 0;
  whiteboard_util_send_method_return(conn, msg,
				     DBUS_TYPE_INT32, &status,
				     WHITEBOARD_UTIL_LIST_END);
  
  dbushandler_send_register_sib(self, conn );
  whiteboard_log_debug_fe();

  return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

static gint dbushandler_initialize(DBusHandler *self)
{
  DBusServer *server = NULL;
  gint name_request_result = 0;
  gint retval = 0;
  DBusError err;
  GString *address = g_string_new("unix:path=");
  address = g_string_append(address, self->local_address);
  DBusObjectPathVTable vtable = { dbushandler_unregister_handler,
                                  dbushandler_handle_message,
                                  NULL, NULL, NULL, NULL};

  whiteboard_log_debug_fb();

  g_return_val_if_fail(NULL != self, -1);

  dbus_error_init(&err);

  if (NULL == (server = dbus_server_listen(address->str, &err)))
    {
      whiteboard_log_error("Could not create DBusServer: %s.\n", err.message);
      whiteboard_log_debug("TODO: error handler!\n");
      retval = -1;
    }
  g_string_free(address, FALSE);

				  
  /* TODO: check if dbushandler_delete should be forwarded here */
  dbus_server_set_new_connection_function(server,
					  dbushandler_handle_connection,
					  self, NULL);
  dbus_error_free(&err);

  if (NULL == (self->session_bus = dbus_bus_get(DBUS_BUS_SESSION, &err)))
    {
      whiteboard_log_error("Could not get session bus.\n");
      whiteboard_log_debug("TODO: error handler!\n");
      retval = -1;
    }

  dbus_error_free(&err);
  name_request_result = dbus_bus_request_name(self->session_bus,
					      SIB_DBUS_SERVICE,
					      DBUS_NAME_FLAG_REPLACE_EXISTING,
					      &err);
  if ( -1 == name_request_result )
    {
      whiteboard_log_error("Could not register name to session bus.\n");
      whiteboard_log_debug("TODO: error handler!\n");
      whiteboard_log_debug("Return flag %d\n", name_request_result);
      retval = -1;
    }

  /* For discovery */
  dbus_connection_setup_with_g_main(self->session_bus,
				    g_main_loop_get_context(self->loop));
  
  if( !dbus_connection_register_object_path( self->session_bus,
					    SIB_DBUS_OBJECT,
					    &vtable,
					    self) )
    {
      whiteboard_log_error("Could not register handlerrs for  session bus.\n");
      retval = -1;
    }
  
  //  dbushandler_handle_connection(NULL, self->session_bus, self);

  /* Send alive message to session bus, this should be caught by existing
   * sinks & sources and they should shutdown or do some tricks to avoid
   * duplicate processes.
   */
  whiteboard_log_debug("Sending alive message to session bus\n");
  whiteboard_util_send_signal(SIB_DBUS_OBJECT,
		       SIB_DBUS_CONTROL_INTERFACE,
		       SIB_DBUS_CONTROL_SIGNAL_STARTING,
		       self->session_bus,
		       WHITEBOARD_UTIL_LIST_END);
  
  dbus_connection_flush(self->session_bus);
  
  /* Point to point connections */
  dbus_server_setup_with_g_main(server,
				g_main_loop_get_context(self->loop));
  
  dbus_error_free(&err);
  
  whiteboard_log_debug_fe();
  
  return retval;
}

static void dbushandler_handle_connection(DBusServer *server,
					  DBusConnection *conn,
					  gpointer data)
{
  DBusHandler *self = (DBusHandler *) data;

  whiteboard_log_debug_fb();
  /* whiteboard_log_print_debug(WHITEBOARD_DEBUG_BEGIN_END, 
		      "%s() END\n",		
		      __FUNCTION__);
  */
  /* TODO: check that connections are 
   * unreferenced correctly on dbushandler delete */
  dbus_connection_ref(conn);

  whiteboard_log_debug("Connection pointer: %p\n", conn);
  g_return_if_fail(NULL != self);

  dbus_connection_add_filter(conn, &dbushandler_handle_message, data, NULL);
  dbus_connection_setup_with_g_main(conn,
				    g_main_loop_get_context(self->loop));

  whiteboard_log_debug_fe();
}

static DBusHandlerResult dbushandler_sib_register_message(DBusHandler* self,
							  DBusConnection* conn,
							  DBusMessage* msg)
{
  const gchar* interface = NULL;
  const gchar* member = NULL;
  gint type = 0;
  DBusHandlerResult result = DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

  whiteboard_log_debug_fb();

  interface = dbus_message_get_interface(msg);
  member = dbus_message_get_member(msg);
  type = dbus_message_get_type(msg);	

  switch (type)
    {
    case DBUS_MESSAGE_TYPE_METHOD_CALL:
      /* Here we create a key -> connection pair for 
       * registered connections and add UI connections to 
       * connection list.
       *
       * Later we use these mappings to find out proper source /
       * sink when routing packets between sources, sinks and 
       * UIs.
       */
      if (!strcmp(member, SIB_DBUS_REGISTER_METHOD_KP))
	{
	  result = dbushandler_register_kp(self, conn, msg);
	}
      else if (!strcmp(member, SIB_DBUS_REGISTER_METHOD_CONTROL))
	{
	  result = dbushandler_register_control(self, conn, msg);
	}
      else
	{
	  whiteboard_log_warning("Method %s not defined "		\
			  " in interface %s", member,
			  interface);
	}
      break;

    case DBUS_MESSAGE_TYPE_METHOD_RETURN:
      whiteboard_log_warning("Return message %s not defined " \
			     " in interface %s", member,
			     interface);
      break;

    case DBUS_MESSAGE_TYPE_ERROR:
      whiteboard_log_warning("Error message %s not defined " \
			     " in interface %s", member,
			     interface);
      break;

    case DBUS_MESSAGE_TYPE_SIGNAL:
      if (!strcmp(member, SIB_DBUS_REGISTER_SIGNAL_UNREGISTER_KP))
	{
	  result = dbushandler_unregister_kp(self, conn, msg);
	}
      else
	{
	  whiteboard_log_warning("Signal message %s not defined "	\
			  " in interface %s", member,
				 interface);
	}
      break;

    default:
      whiteboard_log_error_id(WHITEBOARD_ERROR_UNKNOWN_MESSAGE_TYPE,
			      "Unknown message type: %d\n", type);
      break;
    }

  whiteboard_log_debug_fe();

  return result;
}

static DBusHandlerResult dbushandler_sib_general_message(
								DBusHandler* self, DBusConnection* conn, DBusMessage* msg)
{
  const gchar* interface = NULL;
  const gchar* member = NULL;
  const gchar* connection_name = NULL;
  gint type = 0;
  GString *address;
  gchar *uri = NULL;;
  whiteboard_log_debug_fb();

  interface = dbus_message_get_interface(msg);
  member = dbus_message_get_member(msg);
  type = dbus_message_get_type(msg);
  connection_name = dbus_bus_get_unique_name(conn);

  switch (type)
    {
    case DBUS_MESSAGE_TYPE_METHOD_CALL:
      if (!strcmp(member, SIB_DBUS_METHOD_DISCOVERY))
	{
	  whiteboard_log_debug("Discovery request.\n");
	  address = g_string_new("unix:path=");
	  address = g_string_append(address,self->local_address); 

	  uri = g_strdup(self->my_uri);

	  whiteboard_util_send_method_return(conn, msg, 
					     DBUS_TYPE_STRING, &address->str,
					     DBUS_TYPE_STRING, &uri,
					     WHITEBOARD_UTIL_LIST_END);
	  g_string_free(address,FALSE);
	}
      else
	{
	  whiteboard_log_warning("Method %s not defined " \
				 " in interface %s\n", member,
				 interface);
	}
      break;

    case DBUS_MESSAGE_TYPE_METHOD_RETURN:
      whiteboard_log_warning("Method return %s not defined "		\
				 " in interface %s", member,
				 interface);
      break;

    case DBUS_MESSAGE_TYPE_ERROR:
      whiteboard_log_warning("Error message %s not defined " \
			     " in interface %s", member,
			     interface);
      break;

    case DBUS_MESSAGE_TYPE_SIGNAL:
      whiteboard_log_warning("Signal message %s not defined " \
			     " in interface %s", member,
			     interface);
      break;

    default:
      whiteboard_log_error_id(WHITEBOARD_ERROR_UNKNOWN_MESSAGE_TYPE,
			      "Unknown message type: %d\n", type);
      break;
    }

  return DBUS_HANDLER_RESULT_HANDLED;

  whiteboard_log_debug_fe();
}

static void dbushandler_org_freedesktop_dbus_message(DBusHandler* self,
						     DBusConnection* conn,
						     DBusMessage* msg)
{
  const gchar *interface = NULL;
  const gchar *member = NULL;
  gint type;

  whiteboard_log_debug_fb();
  
  /* TODO? */

  interface = dbus_message_get_interface(msg);
  member = dbus_message_get_member(msg);
  type = dbus_message_get_type(msg);
  
  whiteboard_log_debug("interface: %s\n", interface);
  whiteboard_log_debug("member: %s\n", member);
  
  switch(type)
    {
    case DBUS_MESSAGE_TYPE_METHOD_CALL:
      whiteboard_log_debug("type: method_call\n");
      break;
    case DBUS_MESSAGE_TYPE_METHOD_RETURN:
      whiteboard_log_debug("type: method_return\n");
      break;
    case DBUS_MESSAGE_TYPE_SIGNAL:
      whiteboard_log_debug("type: signal\n");
      break;
	case DBUS_MESSAGE_TYPE_ERROR:
	  whiteboard_log_debug("type: error\n");
	  break;
    }
  
  whiteboard_log_debug_fe();
}

static void dbushandler_org_freedesktop_dbus_local( DBusHandler* self,
						    DBusConnection* conn,
						    DBusMessage* msg)
{
   const gchar* interface = NULL;
  const gchar* member = NULL;
  gint type = 0;
  
  whiteboard_log_debug_fb();
  
  g_return_if_fail( NULL != self );
  g_return_if_fail( NULL != msg );
  
  interface = dbus_message_get_interface(msg);
  member = dbus_message_get_member(msg);
  type = dbus_message_get_type(msg);
  
  if(!strcmp(member, "Disconnected"))
    {
      whiteboard_log_debug("Got Disconnected message\n");
      dbushandler_handle_disconnect( self, conn);
    }
  else
    {
      whiteboard_log_debug("Unknown message on org.freedesktop.DBus.Local interface, member %s\n", member);
    }
  whiteboard_log_debug_fe();
}

static DBusHandlerResult dbushandler_org_freedesktop_dbus_introspectable( DBusHandler* self,
							     DBusConnection* conn,
							     DBusMessage* msg)
{
  DBusHandlerResult retval = DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
   const gchar* interface = NULL;
  const gchar* member = NULL;
  gint type = 0;
  const gchar *introspect_rsp = "";
  whiteboard_log_debug_fb();
  
  g_return_val_if_fail( NULL != self, retval );
  g_return_val_if_fail( NULL != msg, retval );
  
  interface = dbus_message_get_interface(msg);
  member = dbus_message_get_member(msg);
  type = dbus_message_get_type(msg);
  
  if(!strcmp(member, "Introspect"))
    {
      whiteboard_log_debug("Got Introspect\n");
      
      whiteboard_util_send_method_return(conn, msg,
      					 DBUS_TYPE_STRING,
      					 &introspect_rsp,
      					 WHITEBOARD_UTIL_LIST_END);
      retval = DBUS_HANDLER_RESULT_HANDLED;
    }
  else
    {
      whiteboard_log_debug("Unknown message on org.freedesktop.DBus.Local interface, member %s\n", member);
    }
  whiteboard_log_debug_fe();
  return retval;
}

static DBusHandlerResult dbushandler_kp_message(DBusHandler* self,
						DBusConnection* conn,
						DBusMessage* msg)
{
  DBusHandlerResult retval = DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  const gchar* interface = NULL;
  const gchar* member = NULL;
  gint type = 0;
  sib_op_parameter* p;
  GError* gerror = NULL;

  whiteboard_log_debug_fb();
  
  p = g_new0(sib_op_parameter, 1);
      
  g_return_val_if_fail( NULL != self, retval );
  g_return_val_if_fail( NULL != msg, retval );
  
  interface = dbus_message_get_interface(msg);
  member = dbus_message_get_member(msg);
  type = dbus_message_get_type(msg);

  // printf("Got DBUS KP message: %s, type %d\n", member, type);
  /*
   * Dispatch SIB operation handlers in separate threads
   */

  if(!strcmp(member, SIB_DBUS_KP_METHOD_JOIN ))
    {
      whiteboard_log_debug("Got JOIN\n");
      /* DBus message unreferenced in m3_join */
      dbus_message_ref(msg);
      // printf("Ref'd dbus message\n");


      p->msg = msg;
      p->conn = conn;
      p->sib = self->sib_data;
      p->operation = M3_JOIN;
      g_thread_pool_push(self->threadpool, p, &gerror);
      //g_thread_create(m3_join, p, FALSE, &gerror);
      if (gerror)
	{
	  printf("Error creating thread: %s\n", gerror->message);
	  g_error_free(gerror);
	}

      retval = DBUS_HANDLER_RESULT_HANDLED;
    }

  else if(!strcmp(member, SIB_DBUS_KP_METHOD_LEAVE ))
    {
      whiteboard_log_debug("Got LEAVE\n");

      /* DBus message unreferenced in m3_leave */
      dbus_message_ref(msg);

      p->msg = msg;
      p->conn = conn;
      p->sib = self->sib_data;
      p->operation = M3_LEAVE;
      g_thread_pool_push(self->threadpool, p, &gerror);

      //      g_thread_create(m3_leave, p, FALSE, &gerror);
      if (gerror)
	{
	  printf("Error creating thread: %s\n", gerror->message);
	  g_error_free(gerror);
	}
      retval = DBUS_HANDLER_RESULT_HANDLED;
    }
  
  else if(!strcmp(member, SIB_DBUS_KP_METHOD_INSERT ))
    {
      whiteboard_log_debug("Got INSERT\n");

      /* DBus message unreferenced in m3_insert */
      dbus_message_ref(msg);

      p->msg = msg;
      p->conn = conn;
      p->sib = self->sib_data;
      p->operation = M3_INSERT;
      g_thread_pool_push(self->threadpool, p, &gerror);

      //g_thread_create(m3_insert, p, FALSE, &gerror);
      if (gerror)
	{
	  printf("Error creating thread: %s\n", gerror->message);
	  g_error_free(gerror);
	}

      retval = DBUS_HANDLER_RESULT_HANDLED;
    }
  
  else if(!strcmp(member, SIB_DBUS_KP_METHOD_REMOVE ))
    {
      whiteboard_log_debug("Got REMOVE\n");
      
      /* DBus message unreferenced in m3_remove */
      dbus_message_ref(msg);
      
      p->msg = msg;
      p->conn = conn;
      p->sib = self->sib_data;
      p->operation = M3_REMOVE;
      g_thread_pool_push(self->threadpool, p, &gerror);
      
      //g_thread_create(m3_remove, p, FALSE, &gerror);
      if (gerror)
	{
	  printf("Error creating thread: %s\n", gerror->message);
	  g_error_free(gerror);
	}

      retval = DBUS_HANDLER_RESULT_HANDLED;
    }
  
  else if(!strcmp(member, SIB_DBUS_KP_METHOD_UPDATE ))
    {
      whiteboard_log_debug("Got UPDATE\n");

      /* DBus message unreferenced in m3_update */
      dbus_message_ref(msg);

      p->msg = msg;
      p->conn = conn;
      p->sib = self->sib_data;
      p->operation = M3_UPDATE;
      g_thread_pool_push(self->threadpool, p, &gerror);

      //g_thread_create(m3_update, p, FALSE, &gerror);
      if (gerror)
	{
	  printf("Error creating thread: %s\n", gerror->message);
	  g_error_free(gerror);
	}
      retval = DBUS_HANDLER_RESULT_HANDLED;
    }

  else if(!strcmp(member, SIB_DBUS_KP_METHOD_QUERY ))
    {

      whiteboard_log_debug("Got QUERY\n");

      /* DBus message unreferenced in m3_query_spql_upd */
      dbus_message_ref(msg);

      p->msg = msg;
      p->conn = conn;
      p->sib = self->sib_data;
      p->operation = M3_QUERY_SPQL_UPD;
      g_thread_pool_push(self->threadpool, p, &gerror);
      
      //g_thread_create(m3_query_spql_upd, p, FALSE, &gerror);
      if (gerror)
	{
	  printf("Error creating thread: %s\n", gerror->message);
	  g_error_free(gerror);
	}

      retval = DBUS_HANDLER_RESULT_HANDLED;
    }
  else if(!strcmp(member, SIB_DBUS_KP_METHOD_SUBSCRIBE ))
    {
      
      whiteboard_log_debug("Got SUBSCRIBE\n");

      /* DBus message unreferenced in m3_subscribe */
      dbus_message_ref(msg);

      p->msg = msg;
      p->conn = conn;
      p->sib = self->sib_data;
      p->operation = M3_SUBSCRIBE;
      g_thread_pool_push(self->threadpool, p, &gerror);
      
      //g_thread_create(m3_subscribe, p, FALSE, &gerror);
      if (gerror)
	{
	  printf("Error creating thread: %s\n", gerror->message);
	  g_error_free(gerror);
	}

      retval = DBUS_HANDLER_RESULT_HANDLED;
    }
  else if(!strcmp(member, SIB_DBUS_KP_METHOD_UNSUBSCRIBE ))
    {
      
      whiteboard_log_debug("Got UNSUBSCRIBE\n");
      /* DBus message unreferenced in m3_subscribe */
      dbus_message_ref(msg);
      
      p->msg = msg;
      p->conn = conn;
      p->sib = self->sib_data;
      p->operation = M3_UNSUBSCRIBE;
      g_thread_pool_push(self->threadpool, p, &gerror);

      //g_thread_create(m3_unsubscribe, p, FALSE, &gerror);
      if (gerror)
	{
	  printf("Error creating thread: %s\n", gerror->message);
	  g_error_free(gerror);
	}
      retval = DBUS_HANDLER_RESULT_HANDLED;
    }
  else
    {
      whiteboard_log_debug("Unknown message on interface: %s, member: %s\n", interface, member);
    }
  whiteboard_log_debug_fe();
  return retval;
}


static void kp_handler(gpointer data, gpointer userdata)
{
  /* DBusHandler *self = (DBusHandler *)userdata; */
  sib_op_parameter *op = (sib_op_parameter *)data;

  whiteboard_log_debug_fb();
  switch( op->operation)
    {
    case M3_JOIN:
      m3_join(op);
      break;
    case M3_LEAVE:
      m3_leave(op);
      break;
    case M3_INSERT:
      m3_insert(op);
      break;
    case M3_REMOVE:
      m3_remove(op);
      break;
    case M3_UPDATE:
      m3_update(op);
      break;
    case M3_QUERY_SPQL_UPD:
      m3_query_spql_upd(op);
      break;
    case M3_SUBSCRIBE:
      m3_subscribe(op);
      break;
    case M3_UNSUBSCRIBE:
      m3_unsubscribe(op);
      break;
    default:
      whiteboard_log_error("Unknown operation: %d\n", op->operation);
      break;
    }
  whiteboard_log_debug_fe();
}

static void dbushandler_handle_disconnect( DBusHandler* self,
					   DBusConnection* conn)
{
  whiteboard_log_debug_fb();
  // Assume that it was a node connection that left
  // TODO: apply also for SIB access
  rmData *rm = NULL;
  gchar *nodeid = NULL;
  rm = g_new0(rmData,1);
  rm->value = conn;
  
  while( g_hash_table_find(self->connection_map, dbushandler_compare_hashtable_value, rm ) )
    {
      nodeid = g_strdup((gchar *)rm->key);
      
      dbushandler_remove_connection_by_uuid(self, nodeid);
      
      g_free(nodeid);
    }
  g_free(rm);
  whiteboard_log_debug_fe(); 
}

static gboolean dbushandler_compare_hashtable_value(gpointer _key, gpointer _value, gpointer _data)
{
  rmData *rm = (rmData *)_data;
  g_return_val_if_fail(rm != NULL, FALSE);
  if( _value == rm->value)
    {
      rm->key = _key;
      return TRUE;
    }
  return FALSE;
}

static DBusHandlerResult dbushandler_handle_message(DBusConnection *conn,
						    DBusMessage *msg,
						    gpointer data)
{
  DBusHandler* self = NULL;
  const gchar* interface = NULL;
  const gchar* member = NULL;
  const gchar* connection_name = NULL;
  gint type = 0;
  DBusHandlerResult result = DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  whiteboard_log_debug_fb();
  
  self = (DBusHandler *) data;
  
  g_return_val_if_fail(NULL != self, DBUS_HANDLER_RESULT_NOT_YET_HANDLED);

  interface = dbus_message_get_interface(msg);
  member = dbus_message_get_member(msg);
  type = dbus_message_get_type(msg);
  connection_name = dbus_bus_get_unique_name(conn);
  /* printf("dbushandler_handle_message %s %s %d\n", interface,member, type); */
  g_return_val_if_fail(NULL != interface,
		       DBUS_HANDLER_RESULT_NOT_YET_HANDLED);
  g_mutex_lock(self->lock);
  /* TODO: Could be optimized, sender is not needed in many
   * of the routed packets.
   */
  if (!strcmp(interface, SIB_DBUS_KP_INTERFACE))
    {
      whiteboard_log_debug("Got KP packet\n");
      result = dbushandler_kp_message(self,conn,msg);
    }
  else if (!strcmp(interface, SIB_DBUS_REGISTER_INTERFACE))
    {
      whiteboard_log_debug("Got register packet\n");

      //if ( NULL != connection_name )
      //	dbus_message_set_sender(msg, connection_name);

      result = dbushandler_sib_register_message(self, conn, msg);
    }
  else if (!strcmp(interface, SIB_DBUS_INTERFACE))
    {
      whiteboard_log_debug("Got general packet\n");

      /* Don't set the sender because session bus goes wacko */

      dbushandler_sib_general_message(self, conn, msg);
      result = DBUS_HANDLER_RESULT_HANDLED;      
    }
  else if (!strcmp(interface, SIB_DBUS_LOG_INTERFACE))
    {
      GList* kp_connections = dbushandler_get_kp_connections(self); 
	  
      whiteboard_log_debug("Got log message packet\n"); 
	  
      if ( NULL != connection_name ) 
	dbus_message_set_sender(msg, connection_name); 
	  
      whiteboard_util_send_message_to_list(kp_connections, msg);
      result = DBUS_HANDLER_RESULT_HANDLED;      
      
    }
  else if (!strcmp(interface, DBUS_INTERFACE_DBUS))
    {
      printf("Got generic DBus packet\n");
      dbushandler_org_freedesktop_dbus_message(self, conn, msg);
      result = DBUS_HANDLER_RESULT_HANDLED;      
    }
  else if (!strcmp(interface, DBUS_INTERFACE_LOCAL))
    {
      whiteboard_log_debug("Got Local DBus packet\n");
      dbushandler_org_freedesktop_dbus_local( self, conn,msg);
    }
  else if( !strcmp(interface, DBUS_INTERFACE_INTROSPECTABLE ) )
    {
      whiteboard_log_debug("Got Introspectable DBus packet\n");
      result = dbushandler_org_freedesktop_dbus_introspectable( self, conn,msg);
    }
  else
    {
      whiteboard_log_warning("Unknown interface: %s (member: %s)\n", 
			     interface, member);
    }
	
  whiteboard_log_debug_fe();
  g_mutex_unlock(self->lock);
  /* TODO: Check what should be returned here */
  return result;
}

/**************************************************************************
 *
 **************************************************************************/

gint dbushandler_send_register_sib(DBusHandler* self, DBusConnection* conn)
{

  DBusMessage *reply = NULL;
  gchar *uuid = "X";
  gint success = -1;
  gint retval = -1;
  gint parsed = -1;
  whiteboard_log_debug_fb();
  whiteboard_util_send_method_with_reply(SIB_DBUS_SERVICE,
				  SIB_DBUS_OBJECT,
				  SIB_DBUS_CONTROL_INTERFACE,
				  SIB_DBUS_CONTROL_METHOD_REGISTER_SIB,
				  conn,
				  &reply,
				  DBUS_TYPE_STRING,
				  &uuid,
				  WHITEBOARD_UTIL_LIST_END);

  if(reply)
    {
      parsed = whiteboard_util_parse_message(reply,
				      DBUS_TYPE_INT32, &success,
				      DBUS_TYPE_INVALID);
      if(parsed && success)
	{
	  retval = 0;
	}
      
    }
  whiteboard_log_debug_fe();
  return retval;
}

/*****************************************************************************
 * Connection map manipulation
 *****************************************************************************/

void dbushandler_add_connection_by_uuid(DBusHandler* self, gchar* uuid,
					DBusConnection* conn)
{
  whiteboard_log_debug_fb();

  g_return_if_fail(NULL != self);
  g_return_if_fail(NULL != uuid);
  g_return_if_fail(NULL != conn);

  g_hash_table_insert(self->connection_map, g_strdup(uuid), conn);

  whiteboard_log_debugc(WHITEBOARD_DEBUG_DBUS,
			"Insert UUID: %s, conn: %p. Map size: %d\n",
			uuid, conn, g_hash_table_size(self->connection_map));

  whiteboard_log_debug_fe();
}

DBusConnection *dbushandler_get_connection_by_uuid(DBusHandler *self,
						   gchar *uuid)
{
  DBusConnection* conn = NULL;

  whiteboard_log_debug_fb();

  g_return_val_if_fail(NULL != self, NULL);
  g_return_val_if_fail(NULL != uuid, NULL);

  whiteboard_log_debugc(WHITEBOARD_DEBUG_DBUS,
			"Trying to get UUID: %s, Map size: %d\n",
			uuid, g_hash_table_size(self->connection_map));
	
  conn = (DBusConnection*) g_hash_table_lookup(self->connection_map, uuid);

  whiteboard_log_debug_fe();

  return conn;
}

gboolean dbushandler_remove_connection_by_uuid(DBusHandler* self, gchar* uuid)
{
  DBusConnection* conn = NULL;
  gboolean retval = FALSE;

  whiteboard_log_debug_fb();

  g_return_val_if_fail(NULL != self, FALSE);
  g_return_val_if_fail(NULL != uuid, FALSE);

  conn = dbushandler_get_connection_by_uuid(self, uuid);
  if (conn != NULL)
    {
      /* Remove the connection from the hash map */
      retval = g_hash_table_remove(self->connection_map, uuid);

      whiteboard_log_debugc(WHITEBOARD_DEBUG_DBUS,
			    "Removed:%s, conn:%p, ok:%s. Map size:%d\n",
			    uuid, conn, (retval) ? "TRUE" : "FALSE",
			    g_hash_table_size(self->connection_map));

      self->kp_connections = 
	g_list_remove(self->kp_connections, conn);
      
      // dbus_connection_unref(conn);
    }

  whiteboard_log_debug_fe();

  return retval;
}

static void dbushandler_unregister_handler(DBusConnection* conn,gpointer data)
{
  // TODO 
  printf("dbushandler_unregister_handler\n");
}

/* Keep this preprocessor instruction always at the end of the file */
#endif /* UNIT_TEST_INCLUDE_IMPLEMENTATION */
