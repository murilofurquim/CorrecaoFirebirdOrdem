/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		err.cpp
 *	DESCRIPTION:	Bug check routine
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
 *
 * 2002.10.29 Sean Leyne - Removed obsolete "Netware" port
 *
 */

#include "firebird.h"
#include <stdio.h>
#include <string.h>
#include "../jrd/common.h"
#include <stdarg.h>
#include "gen/iberror.h"
#include "../jrd/iberr.h"
#include <errno.h>

#if ( !defined( REQUESTER) && !defined( SUPERCLIENT))
#include "../jrd/jrd.h"
#include "../jrd/os/pio.h"
#include "../jrd/val.h"
#include "../jrd/ods.h"
#include "../jrd/btr.h"
#include "../jrd/req.h"
#include "../jrd/rse.h"
#include "../jrd/tra.h"
#include "../jrd/cch_proto.h"
#include "../jrd/met_proto.h"
#endif
#include "../jrd/thd.h"
#include "../jrd/dbg_proto.h"
#include "../jrd/err_proto.h"
#include "../jrd/gds_proto.h"
#include "../common/config/config.h"
#include "../common/utils_proto.h"

using namespace Firebird;
using namespace Jrd;

//#define JRD_FAILURE_SPACE	2048
//#define JRD_FAILURE_UNKNOWN	"<UNKNOWN>"	/* Used when buffer fails */


static void internal_error(ISC_STATUS status, int number, 
						   const TEXT* file = NULL, int line = 0);
static void internal_post(ISC_STATUS status, va_list args);


#if ( !defined( REQUESTER) && !defined( SUPERCLIENT))
void ERR_bugcheck(int number, const TEXT* file, int line)
{
/**************************************
 *
 *	E R R _ b u g c h e c k
 *
 **************************************
 *
 * Functional description
 *	Things seem to be going poorly today.
 *
 **************************************/
	Database* dbb = GET_DBB();
	dbb->dbb_flags |= DBB_bugcheck;

	CCH_shutdown_database(dbb);

	internal_error(isc_bug_check, number, file, line);
}
#endif


#if ( !defined( REQUESTER) && !defined( SUPERCLIENT))
void ERR_bugcheck_msg(const TEXT* msg)
{
/**************************************
 *
 *	E R R _ b u g c h e c k _ m s g
 *
 **************************************
 *
 * Functional description
 *	Things seem to be going poorly today.
 *
 **************************************/
	Database* dbb = GET_DBB();

	dbb->dbb_flags |= DBB_bugcheck;
	DEBUG;
	CCH_shutdown_database(dbb);

	ERR_post(isc_bug_check, isc_arg_string, ERR_cstring(msg), isc_arg_end);
}
#endif


#if ( !defined( REQUESTER) && !defined( SUPERCLIENT))
void ERR_corrupt(int number)
{
/**************************************
 *
 *	E R R _ c o r r u p t
 *
 **************************************
 *
 * Functional description
 *	Things seem to be going poorly today.
 *
 **************************************/

	internal_error(isc_db_corrupt, number);
}
#endif


const TEXT* ERR_cstring(const TEXT* in_string)
{
/**************************************
 *
 *	E R R _ c s t r i n g
 *
 **************************************
 *
 * Functional description
 *	Copy a stack local string into an area that is
 *	guaranteed to exist for a reasonable length of time.
 *	Only strings that are to be placed in status vectors
 *	should use this routine.
 *	A circular buffer is used to hold these strings.  It
 *	is independent of the JRD allocator mechanism.
 *
 **************************************/
	return Firebird::status_string(in_string);
}


#if ( !defined( REQUESTER) && !defined( SUPERCLIENT))
void ERR_duplicate_error(IDX_E	code,
						const jrd_rel*		relation,
						USHORT index_number,
						const Firebird::string& key_str,
						const TEXT* idx_name)
{
/**************************************
 *
 *	E R R _ d u p l i c a t e _ e r r o r
 *
 **************************************
 *
 * Functional description
 *	Duplicate error during index update.
 *
 **************************************/
	if (code == idx_e_conversion)
		ERR_punt();

	Firebird::MetaName index, constraint;
	const TEXT* index_name;
	const TEXT* constraint_name;

	thread_db* tdbb = JRD_get_thread_data();

	ISC_STATUS_ARRAY org_status;
	memcpy(org_status, tdbb->tdbb_status_vector, sizeof(ISC_STATUS_ARRAY));

	if (!idx_name)
		MET_lookup_index(tdbb, index, relation->rel_name, index_number + 1);
	else
		index = idx_name;

	if (index.length()) {
		index_name = ERR_cstring(index);
		MET_lookup_cnstrt_for_index(tdbb, constraint, index);
		if (constraint.length() > 0)
			constraint_name = ERR_cstring(constraint);
		else
			constraint_name = "***unknown***";
	}
	else {
		index_name = "***unknown***";
		constraint_name = "***unknown***";
	}

	memcpy(tdbb->tdbb_status_vector, org_status, sizeof(ISC_STATUS_ARRAY));

	switch (code) {
	case idx_e_keytoobig:
		ERR_post_nothrow(isc_imp_exc, isc_arg_gds, isc_keytoobig,
				 	 	 isc_arg_string, index_name, isc_arg_end);
		break;

	case idx_e_foreign_target_doesnt_exist:
		ERR_post_nothrow(isc_foreign_key, isc_arg_string, constraint_name,
			 	 	 	 isc_arg_string, ERR_cstring(relation->rel_name),
			 	 	 	 isc_arg_gds, isc_foreign_key_target_doesnt_exist, isc_arg_end);
		break;

	case idx_e_foreign_references_present:
		ERR_post_nothrow(isc_foreign_key, isc_arg_string, constraint_name,
			 	 	 	 isc_arg_string, ERR_cstring(relation->rel_name),
			 	 	 	 isc_arg_gds, isc_foreign_key_references_present, isc_arg_end);
		break;

	case idx_e_duplicate:
		if (constraint.length() > 0)
			ERR_post_nothrow(isc_unique_key_violation,
					 	 	 isc_arg_string, constraint_name,
					 	 	 isc_arg_string, ERR_cstring(relation->rel_name), isc_arg_end);
		else
			ERR_post_nothrow(isc_no_dup, isc_arg_string, index_name, isc_arg_end);
		break;

	default:
		fb_assert(false);
	}

	if (key_str.hasData())
	{
		string errorMsg;
		errorMsg.printf("Problematic key value is %s", key_str.c_str());
		ERR_post_nothrow(isc_random, isc_arg_string, ERR_cstring(errorMsg), isc_arg_end);
	}

	ERR_punt();
}
#endif


#if ( !defined( REQUESTER) && !defined( SUPERCLIENT))
void ERR_error(int number)
{
/**************************************
 *
 *	E R R _ e r r o r
 *
 **************************************
 *
 * Functional description
 *	Post a user-level error.  This is a temporary mechanism
 *	that will eventually disappear.
 *
 **************************************/
	TEXT errmsg[MAX_ERRMSG_LEN + 1];

	DEBUG;
	if (gds__msg_lookup(0, JRD_BUGCHK, number, sizeof(errmsg), errmsg, NULL) < 1)
		sprintf(errmsg, "error code %d", number);

	ERR_post(isc_random, isc_arg_string, ERR_cstring(errmsg), isc_arg_end);
}
#endif


#if ( !defined( REQUESTER) && !defined( SUPERCLIENT))
void ERR_error_msg(const TEXT* msg)
{
/**************************************
 *
 *	E R R _ e r r o r _ m s g
 *
 **************************************
 *
 * Functional description
 *	Post a user-level error.  This is a temporary mechanism
 *	that will eventually disappear.
 *
 **************************************/

	DEBUG;
	ERR_post(isc_random, isc_arg_string, ERR_cstring(msg), isc_arg_end);
}
#endif


#if ( !defined( REQUESTER) && !defined( SUPERCLIENT))
void ERR_log(int facility, int number, const TEXT* message)
{
/**************************************
 *
 *	E R R _ l o g
 *
 **************************************
 *
 * Functional description
 *	Log a message to the firebird.log    
 *
 **************************************/
	TEXT errmsg[MAX_ERRMSG_LEN + 1];
	thread_db* tdbb = JRD_get_thread_data();

	DEBUG;
	if (message) {
		strncpy(errmsg, message, sizeof(errmsg));
		errmsg[sizeof(errmsg) - 1] = 0;
	}
	else
		if (gds__msg_lookup(0, facility, number, sizeof(errmsg), errmsg, NULL) < 1)
			strcpy(errmsg, "Internal error code");

	const size_t len = strlen(errmsg);
	fb_utils::snprintf(errmsg + len, sizeof(errmsg) - len, " (%d)", number);

	gds__log("Database: %s\n\t%s", (tdbb && tdbb->getAttachment()) ?
		tdbb->getAttachment()->att_filename.c_str() : "", errmsg);
}
#endif


#if ( !defined( REQUESTER) && !defined( SUPERCLIENT))
bool ERR_post_warning(ISC_STATUS status, ...)
{
/**************************************
 *
 *      E R R _ p o s t _ w a r n i n g
 *
 **************************************
 *
 * Functional description
 *	Post a warning to the current status vector.
 *
 **************************************/
	va_list args;
	int type, len;
	ISC_STATUS* q;
	int indx = 0, warning_indx = 0;
	ISC_STATUS* status_vector;

	va_start(args, status);
	status_vector = ((thread_db*) JRD_get_thread_data())->tdbb_status_vector;

	if (status_vector[0] != isc_arg_gds ||
		(status_vector[0] == isc_arg_gds && status_vector[1] == 0 &&
		 status_vector[2] != isc_arg_warning))
	{
		/* this is a blank status vector */
		status_vector[0] = isc_arg_gds;
		status_vector[1] = 0;
		status_vector[2] = isc_arg_end;
		indx = 2;
	}
	else {
		/* find end of a status vector */
		PARSE_STATUS(status_vector, indx, warning_indx);
		if (indx)
			--indx;
	}

/* stuff the warning */
	if (indx + 3 < ISC_STATUS_LENGTH) {
		status_vector[indx++] = isc_arg_warning;
		status_vector[indx++] = status;
		while ((type = va_arg(args, int)) && (indx + 3 < ISC_STATUS_LENGTH))
			switch (status_vector[indx++] = type) {
			case isc_arg_warning:
				status_vector[indx++] = (ISC_STATUS) va_arg(args, ISC_STATUS);
				break;

			case isc_arg_string:
				q = reinterpret_cast<ISC_STATUS*>(va_arg(args, TEXT*));
				if (strlen((TEXT *) q) >= (size_t)MAX_ERRSTR_LEN) {
					status_vector[(indx - 1)] = isc_arg_cstring;
					status_vector[indx++] = MAX_ERRSTR_LEN;
				}
				// TEXT* forced to platform's int and stored as ISC_STATUS !!!
				status_vector[indx++] = (ISC_STATUS)(U_IPTR) q;
				break;

			case isc_arg_interpreted:
				status_vector[indx++] = va_arg(args, ISC_STATUS);
				break;

			case isc_arg_cstring:
				len = va_arg(args, int);
				status_vector[indx++] =
					(ISC_STATUS) (len >= MAX_ERRSTR_LEN) ? MAX_ERRSTR_LEN : len;
				// TEXT* forced to platform's int and stored as ISC_STATUS !!!
				status_vector[indx++] = (ISC_STATUS) (U_IPTR) va_arg(args, TEXT*);
				break;

			case isc_arg_number:
				status_vector[indx++] = (ISC_STATUS) va_arg(args, SLONG);
				break;

			case isc_arg_vms:
			case isc_arg_unix:
			case isc_arg_win32:
			default:
				status_vector[indx++] = (ISC_STATUS) va_arg(args, int);
				break;
			}
		status_vector[indx] = isc_arg_end;
		va_end(args);
		return true;
	}
	else {
		/* not enough free space */
		va_end(args);
		return false;
	}
}
#endif


#if ( !defined( REQUESTER) && !defined( SUPERCLIENT))
void ERR_post_nothrow(ISC_STATUS status, ...)
/**************************************
 *
 *	E R R _ p o s t _ n o t h r o w
 *
 **************************************
 *
 * Functional description
 *	Create a status vector.
 *
 **************************************/
{
	va_list args;
	va_start(args, status);
	internal_post(status, args);
	va_end(args);
}


void ERR_post(ISC_STATUS status, ...)
/**************************************
 *
 *	E R R _ p o s t
 *
 **************************************
 *
 * Functional description
 *	Create a status vector and return to 
 * the user.
 *
 **************************************/
{
	va_list args;
	va_start(args, status);
	internal_post(status, args);
	va_end(args);

	DEBUG;
	ERR_punt();
}


static void internal_post(ISC_STATUS status, va_list args)
{
/**************************************
 *
 *	i n t e r n a l _ p o s t
 *
 **************************************
 *
 * Functional description
 *	Append status vector with new values.
 *  Used in ERR_post and ERR_post_nothrow
 *
 **************************************/

	ISC_STATUS* status_vector = ((thread_db*) JRD_get_thread_data())->tdbb_status_vector;

/* stuff the status into temp buffer */
	ISC_STATUS_ARRAY tmp_status;
	MOVE_CLEAR(tmp_status, sizeof(tmp_status));
	STUFF_STATUS_function(tmp_status, status, args);

/* calculate length of the status */
	int tmp_status_len = 0, warning_indx = 0;
	PARSE_STATUS(tmp_status, tmp_status_len, warning_indx);
	fb_assert(warning_indx == 0);

	if (status_vector[0] != isc_arg_gds ||
		(status_vector[0] == isc_arg_gds && status_vector[1] == 0 &&
		 status_vector[2] != isc_arg_warning))
	{
		/* this is a blank status vector just stuff the status */
		MOVE_FASTER(tmp_status, status_vector,
					sizeof(ISC_STATUS) * tmp_status_len);
		return;
	}

	int status_len = 0;
	PARSE_STATUS(status_vector, status_len, warning_indx);
	if (status_len)
		--status_len;

	/* check for duplicated error code */
	int i;
	for (i = 0; i < ISC_STATUS_LENGTH; i++) {
		if (status_vector[i] == isc_arg_end && i == status_len)
			break;				/* end of argument list */

		if (i && i == warning_indx)
			break;				/* vector has no more errors */

		if (status_vector[i] == tmp_status[1] && i &&
			status_vector[i - 1] != isc_arg_warning &&
			i + tmp_status_len - 2 < ISC_STATUS_LENGTH &&
			(memcmp(&status_vector[i], &tmp_status[1],
					sizeof(ISC_STATUS) * (tmp_status_len - 2)) == 0))
		{
			/* duplicate found */
			return;
		}
	}

/* if the status_vector has only warnings then adjust err_status_len */
	int err_status_len = i;
	if (err_status_len == 2 && warning_indx)
		err_status_len = 0;

	ISC_STATUS_ARRAY warning_status;
	int warning_count = 0;
	if (warning_indx) {
		/* copy current warning(s) to a temp buffer */
		MOVE_CLEAR(warning_status, sizeof(warning_status));
		MOVE_FASTER(&status_vector[warning_indx], warning_status,
					sizeof(ISC_STATUS) * (ISC_STATUS_LENGTH - warning_indx));
		PARSE_STATUS(warning_status, warning_count, warning_indx);
	}

/* add the status into a real buffer right in between last error
   and first warning */

	if ((i = err_status_len + tmp_status_len) < ISC_STATUS_LENGTH) {
		MOVE_FASTER(tmp_status, &status_vector[err_status_len],
					sizeof(ISC_STATUS) * tmp_status_len);
		/* copy current warning(s) to the status_vector */
		if (warning_count && i + warning_count - 1 < ISC_STATUS_LENGTH) {
			MOVE_FASTER(warning_status, &status_vector[i - 1],
						sizeof(ISC_STATUS) * warning_count);
		}
	}
	return;
}
#endif


#if ( !defined( REQUESTER) && !defined(SUPERCLIENT))
void ERR_punt(void)
{
/**************************************
 *
 *	E R R _ p u n t
 *
 **************************************
 *
 * Functional description
 *	Error stuff has been copied to status vector.  Now punt.
 *
 **************************************/

	thread_db* tdbb = JRD_get_thread_data();
	Database* dbb = tdbb->getDatabase();

	if (dbb && (dbb->dbb_flags & DBB_bugcheck))
	{
		gds__log_status(tdbb->getAttachment()->att_filename.hasData() ?
			tdbb->getAttachment()->att_filename.c_str() : "Database unknown in ERR_punt on bugcheck",
			tdbb->tdbb_status_vector);
 		if (Config::getBugcheckAbort())
		{
			abort();
		}
	}

	Firebird::status_exception::raise(tdbb->tdbb_status_vector);
}
#endif


const TEXT* ERR_string(const TEXT* in_string, int length)
{
/**************************************
 *
 *	E R R _ s t r i n g
 *
 **************************************
 *
 * Functional description
 *	Copy a stack local string into an area that is
 *	guaranteed to exist for a reasonable length of time.
 *	Only strings that are to be placed in status vectors
 *	should use this routine.
 *	A circular buffer is used to hold these strings.  It
 *	is independent of the JRD allocator mechanism.
 *
 **************************************/
	return Firebird::status_nstring(in_string, length);
}


#if ( !defined( REQUESTER) && !defined( SUPERCLIENT))
void ERR_warning(ISC_STATUS status, ...)
{
/**************************************
 *
 *	E R R _ w a r n i n g
 *
 **************************************
 *
 * Functional description
 *	Write an error out to the status vector but
 *	don't throw an exception.  This allows
 *	sending a warning message back to the user
 *	without stopping execution of a request.  Note
 *	that subsequent errors can supersede this one.
 *
 **************************************/
	thread_db* tdbb = JRD_get_thread_data();

	STUFF_STATUS(tdbb->tdbb_status_vector, status);
	DEBUG;
	tdbb->getRequest()->req_flags |= req_warning;
}
#endif


#if ( !defined( REQUESTER) && !defined( SUPERCLIENT))
static void internal_error(ISC_STATUS status, int number, 
						   const TEXT* file, int line)
{
/**************************************
 *
 *	i n t e r n a l _ e r r o r
 *
 **************************************
 *
 * Functional description
 *	Things seem to be going poorly today.
 *
 **************************************/
	TEXT errmsg[MAX_ERRMSG_LEN + 1];

	DEBUG;
	if (gds__msg_lookup(0, JRD_BUGCHK, number, sizeof(errmsg), errmsg, NULL) < 1)
		strcpy(errmsg, "Internal error code");

	const size_t len = strlen(errmsg);
	
	if (file) {
		// Remove path information
		const TEXT* ptr = (TEXT*)file + strlen(file);
		for (; ptr > file; ptr--) {
			if ((*ptr == '/') || (*ptr == '\\')) {
				ptr++;
				break;
			}
		}
		fb_utils::snprintf(errmsg + len, sizeof(errmsg) - len,
			" (%d), file: %s line: %d", number, ptr, line);
	}
	else {
		fb_utils::snprintf(errmsg + len, sizeof(errmsg) - len, " (%d)", number);
	}

	ERR_post(status, isc_arg_string, ERR_cstring(errmsg), isc_arg_end);
}
#endif

