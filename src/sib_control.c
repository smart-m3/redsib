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
 * SIB daemon
 *
 * sib_control.c
 *
 * Copyright 2007 Nokia Corporation
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <glib.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>

#include <errno.h>

#include <whiteboard_log.h>

#include "sib_control.h"

struct _SibControl
{
	GList *watched_processes;
};

typedef struct _SibProcess
{
	pid_t pid;
	SibProcessState state;
	gchar *executable_name;
	gchar *path;
} SibProcess;

/* Private function declarations */

static gboolean sib_control_spawn_child(SibProcess *process);

static gboolean sib_control_heal_all(SibControl *self);

static gboolean sib_control_heal(SibControl *self,
				      SibProcess *process);

static gboolean sib_control_kill(SibControl *self,
				      SibProcess *process);

/* Public functions */

/**
 * Creates new sib_control instance
 *
 * @return pointer to sib_control instance
 */
SibControl* sib_control_new()
{
	static gboolean instantiated = FALSE;
	SibControl *self = NULL;

	if (instantiated == TRUE)
	{
		/* Can be instantiated only once */
		whiteboard_log_error("SibControl already instantiated. " \
				   "Won't create another instance.\n");
		return NULL;
	}

	self = g_new0(struct _SibControl, 1);

	self->watched_processes = NULL;

	if (NULL != self)
		instantiated = TRUE;

	return self;
}

void sib_control_destroy(SibControl *self)
{
        GList *temp = NULL;

	g_return_if_fail(NULL != self);

	whiteboard_log_debug("Destroying control object.\n");

        for ( temp = self->watched_processes; NULL != temp;
	      temp = g_list_next(temp))
        {
		g_free(((SibProcess *)temp->data)->path);
		g_free(((SibProcess *)temp->data)->executable_name);
		g_free(temp->data);
        }

	g_list_free(self->watched_processes);

	g_free(self);
}

gboolean sib_control_start_all_from(SibControl *self, gchar* path)
{
	GDir *directory;
	const gchar *executable;
	SibProcess *phandle;

	g_return_val_if_fail( NULL != path, FALSE );

	whiteboard_log_debug("Starting all binaries from %s\n", path);

	directory = g_dir_open(path, 0, NULL);
	
	if ( NULL == directory )
	{
		whiteboard_log_error("Could not open directory: %s\n", path);
		return FALSE;
	}

	while ( NULL != (executable = g_dir_read_name(directory)))
	{
		whiteboard_log_debug("Found (hopefully) executable: %s\n",
				   executable);
		phandle = g_new0(SibProcess, 1);
		phandle->path = g_strdup(path);
		phandle->executable_name = g_strdup(executable);
		phandle->state = SIB_PSTATE_INITIALIZED;
		self->watched_processes = g_list_prepend(self->watched_processes,
							 phandle);
	}

	g_dir_close(directory);

	return sib_control_heal_all(self);
}

gboolean sib_control_stop_all(SibControl *self)
{
	g_return_val_if_fail( NULL != self, FALSE );

	gboolean retval = TRUE;
	GList *temp = NULL;

	g_return_val_if_fail( NULL != self, FALSE);

	whiteboard_log_debug("Watchdog: terminating processes.\n");

	for ( temp = self->watched_processes; NULL != temp; 
	      temp = g_list_next(temp))
	{
		retval =  retval && sib_control_kill(self,
							  (SibProcess *)temp->data);
	}

	return retval;
}

/* Private functions */

gboolean sib_control_heal_all(SibControl *self)
{
	gboolean retval = TRUE;
	GList *temp = NULL;

	g_return_val_if_fail( NULL != self, FALSE);

	for ( temp = self->watched_processes; NULL != temp; 
	      temp = g_list_next(temp))
	{
		retval = retval && sib_control_heal(self, 
							 (SibProcess *)temp->data);
	}

	return retval;
}

gboolean sib_control_heal(SibControl *self, SibProcess *process)
{
	gboolean retval = FALSE;

	g_return_val_if_fail( NULL != self, FALSE);
	g_return_val_if_fail( NULL != process, FALSE);

	switch ( process->state )
	{
	case SIB_PSTATE_INITIALIZED:
		whiteboard_log_debug("Process is in initialized state,\n");
		if ( sib_control_spawn_child(process) )
			retval = TRUE;
		break;
	default:
		whiteboard_log_debug("No hanlder for this state yet...\n");
		break;
	}

	return retval;
}

gboolean sib_control_kill(SibControl *self, SibProcess *process)
{
	gboolean retval = FALSE;
	gint status;
	gint options;
	gboolean not_ready_to_exit = TRUE;

	g_return_val_if_fail( NULL != self, FALSE);
	g_return_val_if_fail( NULL != process, FALSE);

	switch ( process->state )
	{

	case SIB_PSTATE_STARTED:
		whiteboard_log_debug(
			"Process is in started state, terminating.\n");

		/* First send termination signal */
		if ( -1 == kill(process->pid, SIGTERM ) ) 
		{
			whiteboard_log_debug(
				"Interrupt failed for process %d: %s\n",
				process->pid, strerror(errno));
		}

		sleep(2);

		/* After giving process 2 seconds to react and exit we
		 * try KILL signal */
		if ( -1 == kill(process->pid, SIGKILL ) ) 
		{
			/* ESRCH means that pid doesn't exist -> go to
			 * waitpid */
			if ( ESRCH == errno ) 
			{
				whiteboard_log_debug("Everything ok, process has "
						   "been terminated "
						   "successfully.\n");
				process->state = SIB_PSTATE_WAITED;
				retval = TRUE;
			}
			else 
			{
				whiteboard_log_error("Process termination failed: "
						   "%s\n", strerror(errno));
			}
		}

		options = WNOHANG;
		while (retval && not_ready_to_exit ) 
		{
			switch ( waitpid(process->pid, &status, options ) )
			{

			case 0:
				whiteboard_log_debug("Hmm. No state changes, "
						   "strange.\n");
				/* This should be possible to happen only
				 * once */
				options = 0;
				break;
			case -1:
				if ( ECHILD == errno ) 
				{
					whiteboard_log_debug(
						"Hmm. Very strange, it "
						"looks like we are trying "
						"to terminate something "
						"not spawned by us.\n");
				}
				else 
				{
					whiteboard_log_debug(
						"Error waiting child: " \
						"%s\n", 
						strerror(errno));

				}
				not_ready_to_exit = FALSE;
				break;
			default:
				if (WIFSIGNALED(status)) 
				{
					whiteboard_log_debug(
						"Process was correctly "
						"terminated by signal.\n");
				}
				process->state = SIB_PSTATE_TERMINATED;
				process->pid = -1;
				not_ready_to_exit = FALSE;
				break;
			}
		}

		break;
	default:
		whiteboard_log_debug("No hanlder for this state yet...\n");
		break;

	}

	return retval;
}

gboolean sib_control_spawn_child(SibProcess *process)
{
	gboolean retval = FALSE;

	g_return_val_if_fail( NULL != process, FALSE);
	gchar *cmd[3];

	whiteboard_log_debug("Forking...\n");
	process->pid = fork();

	switch ( process->pid )
	{
	case -1:
		whiteboard_log_error("Fork failed: %s\n", strerror(errno));
		break;
	case 0:
		whiteboard_log_debug("Child: running\n");
#ifdef USE_MAEMO		
		cmd[0] = SIB_CONTROL_EXEC_SCRIPT;
		cmd[1] = g_strconcat(process->path, "/",
				     process->executable_name, NULL);
		cmd[2] = (char *)0;

		whiteboard_log_trace("Child: starting Access process: %s\n",
				   process->executable_name);

		if ( -1 == execvp(SIB_CONTROL_EXEC_SCRIPT, cmd) )
		{
			whiteboard_log_error(
				"Could not spawn child process: %s\n",
				strerror(errno));
			exit(0);
		}
#else
		cmd[0] = g_strconcat(process->path, "/",
				     process->executable_name, NULL);
		cmd[1] = (char *)0;
		cmd[2]= (char*)0;

		whiteboard_log_trace("Child: starting Access process: %s\n",
				   process->executable_name);
		whiteboard_log_trace("Command %s\n",
				     cmd[0]);
		if ( -1 == execv(cmd[0], cmd) )
		{
			whiteboard_log_error(
				"Could not spawn child process: %s\n",
				strerror(errno));
			exit(0);
		}
#endif
		
		
		break;
	default:
		whiteboard_log_debug(
			"Parent: marking process as started.\n");
		process->state = SIB_PSTATE_STARTED;
		retval = TRUE;
		break;
	}

	return retval;
}
