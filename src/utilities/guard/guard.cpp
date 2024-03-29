/*
 * The contents of this file are subject to the Interbase Public
 * License Version 1.0 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy
 * of the License at http://www.Inprise.com/IPL.html
 *
 * Software distributed under the License is distributed on an
 * "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either express
 * or implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code was created by Inprise Corporation
 * and its predecessors. Portions created by Inprise Corporation are
 * Copyright (C) Inprise Corporation.
 *
 * All Rights Reserved.
 * Contributor(s): ______________________________________.
 *
 */
 /* contains the main() and not shared routines for ibguard */


#include "firebird.h"
#include <stdio.h>

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#ifdef HAVE_ERRNO_H
#include <errno.h>
#else
int errno = -1;
#endif

#include <time.h>

#include "../jrd/common.h"
#include "../jrd/divorce.h"
#include "../jrd/isc_proto.h"
#include "../jrd/gds_proto.h"
#include "../jrd/file_params.h"
#include "../utilities/guard/util_proto.h"
#include "../common/classes/fb_string.h"

const USHORT FOREVER	= 1;
const USHORT ONETIME	= 2;
const USHORT IGNORE		= 3;
const USHORT NORMAL_EXIT= 0;

const char* SUPER_SERVER_BINARY	= "bin/fbserver";

const char* INTERBASE_USER		= "interbase";
const char* FIREBIRD_USER		= "firebird";
const char* INTERBASE_USER_SHORT= "interbas";

volatile sig_atomic_t shutting_down;


void shutdown_handler(int)
{
	shutting_down = 1;
}


int CLIB_ROUTINE main( int argc, char **argv)
{
/**************************************
 *
 *      m a i n
 *
 **************************************
 *
 * Functional description
 *      The main for ibguard. This process is used to start
 *      the super server (fbserver) and keep it running
 *	after an abnormal termination.
 *
 *      process takes 1 argument:  -f (default) or -o
 *
 **************************************/
	USHORT option = FOREVER;	/* holds FOREVER or ONETIME  or IGNORE */
	bool done = true;
	const TEXT* prog_name = argv[0];
	const TEXT* pidfilename = 0;
	int guard_exit_code = 0;

	const TEXT* const* const end = argc + argv;
	argv++;
	while (argv < end) {
		const TEXT* p = *argv++;
		if (*p++ == '-')
			switch (UPPER(*p)) {
			case 'F':
				option = FOREVER;
				break;
			case 'O':
				option = ONETIME;
				break;
			case 'S':
				option = IGNORE;
				break;
			case 'P':
				pidfilename = *argv++;
				break;
			default:
				fprintf(stderr,
						   "Usage: %s [-signore | -onetime | -forever (default)] [-pidfile filename]\n",
						   prog_name);
				exit(-1);
				break;
			}

	}							/* while */

/* check user id */
	Firebird::string user_name;		/* holds the user name */
	ISC_get_user(&user_name, NULL, NULL, NULL);

	if (user_name != INTERBASE_USER && 
		user_name != "root" &&
		user_name != FIREBIRD_USER &&
		user_name != INTERBASE_USER_SHORT)
	{
		/* invalid user bail out */
		fprintf(stderr,
				   "%s: Invalid user (must be %s, %s, %s or root).\n",
				   prog_name, FIREBIRD_USER, INTERBASE_USER,
				   INTERBASE_USER_SHORT);
		exit(-2);
	}

/* get and set the umask for the current process */
	const ULONG new_mask = 0000;
	const ULONG old_mask = umask(new_mask);

/* exclusive lock the file */
	int fd_guard;
	if ((fd_guard = UTIL_ex_lock(GUARD_FILE)) < 0) {
		/* could not get exclusive lock -- some other guardian is running */
		if (fd_guard == -2)
			fprintf(stderr, "%s: Program is already running.\n",
					   prog_name);
		exit(-3);
	}

/* the umask back to orignal donot want to carry this to child process */
	umask(old_mask);

/* move the server name into the argument to be passed */
	TEXT process_name[1024];
	process_name[0] = '\0';
	TEXT* server_args[2];
	server_args[0] = process_name;
	server_args[1] = NULL;

	shutting_down = 0;
	if (UTIL_set_handler(SIGTERM, shutdown_handler, false) < 0) {
		fprintf(stderr, "%s: Cannot set signal handler (error %d).\n",
			prog_name, errno);
		exit(-5);
	}
	if (UTIL_set_handler(SIGINT, shutdown_handler, false) < 0) {
		fprintf(stderr, "%s: Cannot set signal handler (error %d).\n",
			prog_name, errno);
		exit(-5);
	}

// detach from controlling tty
	divorce_terminal(0);

	time_t timer = 0;

	do {
		int ret_code;

		if (shutting_down) {
			// don't start a child
			break;
		}

		if (timer == time(0))
		{
			// don't let fbserver restart too often - avoid log overflow
			sleep(1);
			continue;
		}
		timer = time(0);

		gds__log("%s: guardian starting %s\n",
				 prog_name, SUPER_SERVER_BINARY);
		pid_t child_pid = UTIL_start_process(SUPER_SERVER_BINARY, server_args);
		if (child_pid == -1) {
			/* could not fork the server */
			gds__log("%s: guardian could not start %s\n",
				prog_name, process_name);
			fprintf(stderr, "%s: Could not start %s\n",
				prog_name, process_name);
			UTIL_ex_unlock(fd_guard);
			exit(-4);
		}
		
		if (pidfilename) {
			FILE *pf = fopen(pidfilename, "w");
			if (pf)
			{
				fprintf(pf, "%d", child_pid);
				fclose(pf);
			}
			else {
				gds__log("%s: guardian could not open %s for writing, error %d\n",
						 prog_name, pidfilename, errno);
			}
		}

		/* wait for child to die, and evaluate exit status */
		bool shutdown_child = true;
		if (!shutting_down) {
			ret_code = UTIL_wait_for_child(child_pid, shutting_down);
			shutdown_child = (ret_code == -2);
		}
		if (shutting_down) {
			if (shutdown_child) {
				ret_code = UTIL_shutdown_child(child_pid, 3, 1);
				if (ret_code < 0) {
					gds__log(
						"%s: error while shutting down %s (%d)\n",
						prog_name, process_name, errno);
					guard_exit_code = -6;
				}
				else if (ret_code == 1) {
					gds__log(
						"%s: %s killed (did not terminate)\n",
						prog_name, process_name);
				}
				else if (ret_code == 2) {
					gds__log(
						"%s: unable to shutdown %s\n",
						prog_name, process_name);
				}
				else {
					gds__log(
						"%s: %s terminated\n",
						 prog_name, process_name);
				}
			}
			break;
		}
		if (ret_code != NORMAL_EXIT) {
			/* check for startup error */
			if (ret_code == STARTUP_ERROR) {
				gds__log("%s: %s terminated due to startup error (%d)\n",
						 prog_name, process_name, ret_code);
				if (option == IGNORE) {
					gds__log
						("%s: %s terminated due to startup error (%d)\n Trying again\n",
						 prog_name, process_name, ret_code);

					done = false;	/* Try it again, Sam (even if it is a startup error) FSG 8.11.2000 */
				}
				else {
					gds__log("%s: %s terminated due to startup error (%d)\n",
							 prog_name, process_name, ret_code);

					done = true;	/* do not restart we have a startup problem */
				}
			}
			else {
				gds__log("%s: %s terminated abnormally (%d)\n",
						prog_name, process_name, ret_code);
				if (option == FOREVER || option == IGNORE)
					done = false;
			}
		}
		else {
			/* Normal shutdown - eg: via ibmgr - don't restart the server */
			gds__log("%s: %s normal shutdown.\n",
					prog_name, process_name); 
			done = true;
		}
	} while (!done);

	if (pidfilename) {
		remove(pidfilename);
	}
	UTIL_ex_unlock(fd_guard);
	exit(guard_exit_code);
}								/* main */
