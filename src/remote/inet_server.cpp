/*
 *	PROGRAM:	JRD Remote Server
 *	MODULE:		inet_server.cpp
 *	DESCRIPTION:	Internet remote server.
 *
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
 * 2002.10.28 Sean Leyne - Code cleanup, removed obsolete "MPEXL" port
 * 2002.10.28 Sean Leyne - Code cleanup, removed obsolete "DecOSF" port
 * 2002.10.28 Sean Leyne - Code cleanup, removed obsolete "SGI" port
 *
 * 2002.10.29 Sean Leyne - Removed obsolete "Netware" port
 *
 * 2002.10.30 Sean Leyne - Removed support for obsolete "PC_PLATFORM" define
 *
 */

#include "firebird.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "../jrd/common.h"
#include "../jrd/isc_proto.h"
#include "../jrd/divorce.h"
#include "../jrd/jrd_proto.h"
#include "../common/config/config.h"
#if !(defined VMS)
#include <sys/param.h>
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#if HAVE_SYS_WAIT_H
# include <sys/wait.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif


#ifdef VMS
#include <descrip.h>
#endif

#ifdef SUPERSERVER
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <errno.h>
#include "../jrd/ibase.h"
#include "../jrd/jrd_pwd.h"
#endif

#include "../remote/remote.h"
#include "../jrd/license.h"
#include "../jrd/thd.h"
#include "../jrd/file_params.h"
#include "../remote/inet_proto.h"
#include "../remote/serve_proto.h"
#include "../jrd/gds_proto.h"
#include "../jrd/sch_proto.h"
#include "../jrd/thread_proto.h"
#include "../common/utils_proto.h"
#include "../common/classes/fb_string.h"

#ifdef UNIX
#ifdef NETBSD
#include <signal.h>
#else
#include <sys/signal.h>
#endif
#endif

#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif

#if (defined SUPERSERVER && defined UNIX && defined SERVER_SHUTDOWN)
#include "../common/classes/semaphore.h"
#define SHUTDOWN_THREAD
#endif

#ifdef UNIX
const char* TEMP_DIR = "/tmp";
#define CHANGE_DIR chdir
#endif

#ifdef SUPERSERVER
const char* INTERBASE_USER_NAME		= "interbase";
const char* INTERBASE_USER_SHORT	= "interbas";
const char* FIREBIRD_USER_NAME		= "firebird";
#endif


#ifdef VMS
static int assign(SCHAR *);
#else
static void set_signal(int, void (*)(int));
#endif
static void signal_handler(int);

#if (defined SUPERSERVER && defined UNIX )
//static void signal_sigpipe_handler(int);
#endif

#ifdef SHUTDOWN_THREAD
static THREAD_ENTRY_DECLARE shutdown_thread(THREAD_ENTRY_PARAM arg);
static void signal_term(int);
static void shutdown_init();
static void shutdown_fini();
#endif

static TEXT protocol[128];
static int INET_SERVER_start = 0;
static USHORT INET_SERVER_flag = 0;

extern "C" {

int FB_EXPORTED server_main( int argc, char** argv)
{
/**************************************
 *
 *	m a i n
 *
 **************************************
 *
 * Functional description
 *	Run the server with apollo mailboxes.
 *
 **************************************/
	rem_port* port;

// 01 Sept 2003, Nickolay Samofatov
// In GCC version 3.1-3.3 we need to install special error handler
// in order to get meaningful terminate() error message on stderr. 
// In GCC 3.4 or later this is the default.
//#if __GNUC__ == 3 && __GNUC_MINOR__ >= 1 && __GNUC_MINOR__ < 4
//    std::set_terminate (__gnu_cxx::__verbose_terminate_handler);
//#endif

#ifdef VMS
	argc = VMS_parse(&argv, argc);
#endif

	const TEXT* const* const end = argc + argv;
	argv++;
	bool debug = false, standalone = false;
	INET_SERVER_flag = 0;
	int channel = 0;
	protocol[0] = 0;
	bool multi_client = false, multi_threaded = false;

#ifdef SUPERSERVER
	INET_SERVER_flag |= SRVR_multi_client;
	multi_client = multi_threaded = standalone = true;
#endif

	int clients = 0;
	bool done = false;

	while (argv < end) {
		TEXT c;
		const TEXT* p = *argv++;
		if (*p++ == '-')
			while (c = *p++) {
				switch (UPPER(c)) {
				case 'D':
					INET_SERVER_flag |= SRVR_debug;
					debug = standalone = true;
					break;
#ifndef SUPERSERVER

				case 'M':
					INET_SERVER_flag |= SRVR_multi_client;
					if (argv < end)
					{
						if (clients = atoi(*argv))
							argv++;
					}
					multi_client = standalone = true;
					break;

				case 'S':
					standalone = true;
					break;


				case 'I':
					standalone = false;
					break;

				case 'T':
					multi_threaded = true;
					break;

				case 'U':
					multi_threaded = false;
					break;
#endif /* SUPERSERVER */

				case 'E':
					if (ISC_set_prefix(p, *argv) == -1)
						printf("Invalid argument Ignored\n");
					else
						argv++;	/* do not skip next argument if this one
								   is invalid */
					done = true;
					break;

				case 'P':
					fb_utils::snprintf(protocol, sizeof(protocol), "/%s", *argv++);
					break;

                case 'H':
				case '?':
					printf("Firebird TCP/IP server options are:\n");
					printf("  -d           : debug on\n");
                   
#ifdef SUPERSERVER
                    // These options only applicable to super server
					printf("  -m           : multiclient - on\n");
					printf("  -s           : standalone - true\n");
					printf("  -i           : standalone - false\n");

					printf("  -t           : multithread - true  (non pc only)\n");
					printf("  -u           : multithread - false (pc only)\n");
					printf("  -t           : multithread (non pc only\n");
#endif

					printf("  -p<protocol> : specify protocol\n");
					printf("  -h|? : print this help\n");
                    printf("\n");
                    printf("  (The following -e options used to be -h options)\n");
					printf("  -e <firebird_root_dir>   : set firebird_root path\n");            
					printf("  -el <firebird_lock_dir>  : set runtime firebird_lock dir\n");            
					printf("  -em <firebird_msg_dir>   : set firebird_msg dir path\n");            
					printf("  -z   : print version\n");            

					exit(FINI_OK);
				case 'Z':
					printf("Firebird TCP/IP server version %s\n",
							  GDS_VERSION);
					exit(FINI_OK);
				}
				if (done)
					break;
			}
	}

	// activate paths set with -e family of switches
	ISC_set_prefix(0, 0);

#if (defined SUPERSERVER && defined UNIX )
	/* set_signal(SIGPIPE, signal_sigpipe_handler); */
	set_signal(SIGPIPE, signal_handler);
	set_signal(SIGUSR1, signal_handler);
	set_signal(SIGUSR2, signal_handler);
#endif

#if defined(UNIX) && defined(HAVE_SETRLIMIT) && defined(HAVE_GETRLIMIT)
#if !(defined(DEV_BUILD))
	if (Config::getBugcheckAbort())
#endif
	{
		// try to force core files creation
		struct rlimit core;
		if (getrlimit(RLIMIT_CORE, &core) == 0)
		{
			core.rlim_cur = core.rlim_max;
			if (setrlimit(RLIMIT_CORE, &core) != 0)
			{
				gds__log("setrlimit() failed, errno=%d", errno);
			}
		}
		else
		{
			gds__log("getrlimit() failed, errno=%d", errno);
		}

		// we need some writable directory for core file
		// on any unix /tmp seems to be the best place
		if (CHANGE_DIR(TEMP_DIR)) {
			/* error on changing the directory */
			gds__log("Could not change directory to %s due to errno %d",
					TEMP_DIR, errno);
		}
	}
#endif

/* Fork off a server, wait for it to die, then fork off another,
   but give up after 100 tries */

#if !(defined SUPERSERVER || defined VMS)
	if (multi_client && !debug) {
		set_signal(SIGUSR1, signal_handler);
		int child;
		for (int n = 0; n < 100; n++) {
			INET_SERVER_start = 0;
			if (!(child = fork()))
				break;
			while (wait(0) != child)
				if (INET_SERVER_start) {
					n = 0;		/* reset error counter on "real" signal */
					break;
				}
			gds__log("INET_SERVER/main: gds_inet_server restarted");
		}
		set_signal(SIGUSR1, SIG_DFL);
	}
#endif

	if (standalone) {
		if (multi_client) {
#ifdef SUPERSERVER

            // Remove restriction on username, for DEV builds
            // restrict only for production builds.  MOD 21-July-2002
#ifndef DEV_BUILD
			Firebird::string user_name;	/* holds the user name */
			/* check user id */
			ISC_get_user(&user_name, NULL, NULL, NULL);

			if (user_name != "root" &&
				user_name != FIREBIRD_USER_NAME &&
				user_name != INTERBASE_USER_NAME &&
				user_name != INTERBASE_USER_SHORT)
			{
				/* invalid user -- bail out */
				fprintf(stderr,
						   "%s: Invalid user (must be %s, %s, %s or root).\n",
						   "fbserver", FIREBIRD_USER_NAME,
						   INTERBASE_USER_NAME, INTERBASE_USER_SHORT);
				exit(STARTUP_ERROR);
			}
#endif
#else
			if (setreuid(0, 0) < 0)
				printf("Inet_server: couldn't set uid to superuser.\n");
#endif
			INET_set_clients(clients);
		}

		if (!debug) {
			int mask = 0; // FD_ZERO(&mask);
			mask |= 1 << 2; // FD_SET(2, &mask);
			divorce_terminal(mask);
		}
		{ // scope block
			ISC_STATUS_ARRAY status_vector;
			THREAD_ENTER();
			port = INET_connect(protocol, 0, status_vector, INET_SERVER_flag,
								0, 0);
			THREAD_EXIT();
			if (!port) {
				gds__print_status(status_vector);
				exit(STARTUP_ERROR);
			}
		} // end scope block
	}
	else {
#ifdef VMS
		channel = assign("SYS$INPUT");
#endif
		THREAD_ENTER();
		port = INET_server(channel);
		THREAD_EXIT();
		if (!port) {
			fprintf(stderr, "fbserver: Unable to start INET_server\n");
			exit(STARTUP_ERROR);
		}
	}

#ifdef SUPERSERVER

/* before starting the superserver stuff change directory to tmp */
	if (CHANGE_DIR(TEMP_DIR)) {
		/* error on changing the directory */
		gds__log("Could not change directory to %s due to errno %d",
				TEMP_DIR, errno);
	}

/* Server tries to attach to security2.fdb to make sure everything is OK
   This code fixes bug# 8429 + all other bug of that kind - from 
   now on the server exits if it cannot attach to the database
   (wrong or no license, not enough memory, etc.
*/
	{
		TEXT path[MAXPATHLEN];
		ISC_STATUS_ARRAY status;
		isc_db_handle db_handle = 0L;

		SecurityDatabase::getPath(path);
		const char dpb[] = {isc_dpb_version1, isc_dpb_gsec_attach, 1, 1};
		isc_attach_database(status, strlen(path), path, &db_handle, sizeof dpb, dpb);
		if (status[0] == 1 && status[1] > 0) {
			gds__log_status(path, status);
			isc_print_status(status);
			exit(STARTUP_ERROR);
		}
		isc_detach_database(status, &db_handle);
		if (status[0] == 1 && status[1] > 0) {
			gds__log_status(path, status);
			isc_print_status(status);
			exit(STARTUP_ERROR);
		}
	}

#endif

#ifdef SHUTDOWN_THREAD
	shutdown_init();
#endif

	if (multi_threaded)
		SRVR_multi_thread(port, INET_SERVER_flag);
	else
		SRVR_main(port, INET_SERVER_flag);

#ifdef SHUTDOWN_THREAD
	shutdown_fini();
#endif

#ifdef DEBUG_GDS_ALLOC
/* In Debug mode - this will report all server-side memory leaks
 * due to remote access
 */
	//gds_alloc_report(0, __FILE__, __LINE__);
	char name[MAXPATHLEN];
	gds__prefix(name, "memdebug.log");
	FILE* file = fopen(name, "w+t");
	if (file) {
	  fprintf(file, "Global memory pool allocated objects\n");
	  getDefaultMemoryPool()->print_contents(file);
	  fclose(file);
	}
#endif

	exit(FINI_OK);
}

}


#ifdef VMS
static int assign( SCHAR * string)
{
/**************************************
 *
 *	a s s i g n
 *
 **************************************
 *
 * Functional description
 *	Assign a channel for communications.
 *
 **************************************/
	SSHORT channel;
	struct dsc$descriptor_s desc;

	desc.dsc$b_dtype = DSC$K_DTYPE_T;
	desc.dsc$b_class = DSC$K_CLASS_S;
	desc.dsc$w_length = strlen(string);
	desc.dsc$a_pointer = string;

	int status = sys$assign(&desc, &channel, NULL, NULL);

	return (status & 1) ? channel : 0;
}
#endif





#if !(defined VMS)
static void set_signal( int signal_number, void (*handler) (int))
{
/**************************************
 *
 *	s e t _ s i g n a l
 *
 **************************************
 *
 * Functional description
 *	Establish signal handler.
 *
 **************************************/
	struct sigaction vec, old_vec;

	vec.sa_handler = handler;
	sigemptyset(&vec.sa_mask);
	vec.sa_flags = 0;
	sigaction(signal_number, &vec, &old_vec);
}
#endif


static void signal_handler(int)
{
/**************************************
 *
 *	s i g n a l _ h a n d l e r
 *
 **************************************
 *
 * Functional description
 *	Dummy signal handler.
 *
 **************************************/

	++INET_SERVER_start;
}

#ifdef NOT_USED_OR_REPLACED
#if (defined SUPERSERVER && defined UNIX )
static void signal_sigpipe_handler(int)
{
/****************************************************
 *
 *	s i g n a l _ s i g p i p e _ h a n d l e r
 *
 ****************************************************
 *
 * Functional description
 *	Dummy signal handler.
 *
 **************************************/

	++INET_SERVER_start;
	gds__log
		("Super Server/main: Bad client socket, send() resulted in SIGPIPE, caught by server\n                   client exited improperly or crashed ????");
}
#endif //SUPERSERVER && UNIX
#endif

#ifdef SHUTDOWN_THREAD
static Firebird::SignalSafeSemaphore shutSem;
static bool alreadyClosing = false;

static THREAD_ENTRY_DECLARE shutdown_thread(THREAD_ENTRY_PARAM arg) 
{
/****************************************************
 *
 *	s h u t d o w n _ t h r e a d
 *
 ****************************************************
 *
 * Functional description
 *	In order to avoid blocking of the thread, 
 *	which received SIGTERM, run in separate thread.
 *
 **************************************/
	try {
	 	shutSem.enter();
	}
	catch (Firebird::status_exception& e)
	{
		TEXT buffer[1024];
        const ISC_STATUS* vector = e.value();
		if (! (vector && fb_interpret(buffer, sizeof(buffer), &vector)))
		{
			strcpy(buffer, "Unknown failure in shutdown thread in shutSem:enter()");
		}
		gds__log("%s", buffer);
		exit(0);
	}
	if (! alreadyClosing)
	{
		alreadyClosing = true;
		JRD_shutdown_all(false);
		SRVR_shutdown();
		exit(0);
	}
	return 0;	//make compilers happy
}

static void signal_term(int)
{
/****************************************************
 *
 *	s i g n a l _ t e r m
 *
 ****************************************************
 *
 * Functional description
 *	Handle ^C and kill.
 *
 **************************************/
	try
	{
	 	shutSem.release();
	}
	catch (Firebird::status_exception& e)
	{
		TEXT buffer[1024];
        const ISC_STATUS* vector = e.value();
		if (! (vector && fb_interpret(buffer, sizeof(buffer), &vector)))
		{
			strcpy(buffer, "Unknown failure in semaphore::release()");
		}
		gds__log("%s", buffer);
		exit(0);
	}
}

static void shutdown_init()
{
	gds__thread_start(shutdown_thread, 0, THREAD_medium, 0, 0);
	// process signals 2 & 15 in order to exit gracefully
	set_signal(SIGINT, signal_term);
	set_signal(SIGTERM, signal_term);
}

static void shutdown_fini()
{
	set_signal(SIGINT, SIG_IGN);
	set_signal(SIGTERM, SIG_IGN);
	alreadyClosing = true;
	shutSem.release();
}
#endif //SHUTDOWN_THREAD
