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
 * Sib Daemon
 *
 * sib_control.h
 *
 * Copyright 2007 Nokia Corporation
 */

#ifndef SIB_CONTROL_H
#define SIB_CONTROL_H

struct _SibControl;
typedef struct _SibControl SibControl;

#define SIB_CONTROL_EXEC_SCRIPT "run-standalone.sh"

typedef enum
{
	SIB_PSTATE_UNKNOWN,

	/* Active states */
	SIB_PSTATE_RUNNING,
	SIB_PSTATE_SLEEPING,
	SIB_PSTATE_DISK,
	SIB_PSTATE_ZOMBIE,
	SIB_PSTATE_TRACED,
	SIB_PSTATE_SSTOPPED,
	SIB_PSTATE_PAGING,

	/* Non-active states */
	SIB_PSTATE_INITIALIZED,
	SIB_PSTATE_STARTED,
	SIB_PSTATE_STOPPED,
	SIB_PSTATE_WAITED,
	SIB_PSTATE_TERMINATED,
	SIB_PSTATE_KILLED
} SibProcessState;

/**
 * Creates new sib_control instance
 *
 * @return pointer to sib_control instance
 */
SibControl *sib_control_new(void);

/**
 * Frees memory allocated by sib_control_new
 * and *start* calls.
 *
 * @param self Pointer to sib_control
 */
void sib_control_destroy(SibControl *self);

/**
 * Callback definition for process state changes
 */
typedef void (*SibProcessInfo) (SibControl* context,
				     gchar *uuid,
				     SibProcessState state,
				     gpointer user_data);

/**
 * Starts all runnable binaries under provided path and
 * stores information about them for future use.
 *
 * @param path Path to executables
 *
 * @return TRUE if processes could be started false otherwise
 */
gboolean sib_control_start_all_from(SibControl *self, gchar* path);

/**
 * Tries to stop all watched processes
 *
 * @return TRUE if all processes were terminated successfully
 */
gboolean sib_control_stop_all(SibControl *self);

#endif /* SIB_CONTROL_H */
