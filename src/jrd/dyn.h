/*
 *	PROGRAM:	JRD Data Definition Utility
 *	MODULE:		dyn.h
 *	DESCRIPTION:	Dynamic data definition local data
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
 */

#ifndef	JRD_DYN_H
#define JRD_DYN_H

#include "../common/classes/MsgPrint.h"

const char* const ALL_PRIVILEGES = "SIUDR";
		/* all applicable grant/revoke privileges */
const char* const ALL_PROC_PRIVILEGES = "X";
		/* all applicable grant/revoke privileges for a procedure */
const int DYN_MSG_FAC		= 8;
const int STUFF_COUNT		= 4;
const int TEXT_BLOB_LENGTH	= 512;


#define GET_STRING(from, to)	DYN_get_string ((const TEXT**)from, to, sizeof(to), true)
#define GET_BYTES(from, to)		DYN_get_string ((const TEXT**)from, to, sizeof(to), false)

namespace Jrd {

struct bid;
class jrd_tra;

class Global
{
public:
	explicit Global(jrd_tra* t) : gbl_transaction(t) { }
	jrd_tra* gbl_transaction;
};

class dyn_fld {
public:
	dsc dyn_dsc;
	bool dyn_null_flag;
	USHORT dyn_dtype;
	USHORT dyn_precision;
	USHORT dyn_charlen;
	SSHORT dyn_collation;
	SSHORT dyn_charset;
	Firebird::MetaName dyn_fld_source;
	Firebird::MetaName dyn_rel_name;
	Firebird::MetaName dyn_fld_name;
    USHORT dyn_charbytelen; // Used to check modify operations on string types.
    const UCHAR* dyn_default_src;
    const UCHAR* dyn_default_val;
    bool dyn_drop_default;
public:
	explicit dyn_fld(MemoryPool& p) 
		: dyn_null_flag(false), dyn_dtype(0), dyn_precision(0), dyn_charlen(0), 
		dyn_collation(0), dyn_charset(0), dyn_fld_source(p), dyn_rel_name(p),
		dyn_fld_name(p), dyn_charbytelen(0),
		dyn_default_src(0), dyn_default_val(0), dyn_drop_default(false) { }
	dyn_fld()
		: dyn_null_flag(false), dyn_dtype(0), dyn_precision(0), dyn_charlen(0), 
		dyn_collation(0), dyn_charset(0), dyn_charbytelen(0),
		dyn_default_src(0), dyn_default_val(0), dyn_drop_default(false) { }
};

} //namespace Jrd

void	DYN_error(bool, USHORT, const MsgFormat::SafeArg& sarg = MsgFormat::SafeArg());
void	DYN_error_punt(bool, USHORT, const MsgFormat::SafeArg& arg);
void	DYN_error_punt(bool, USHORT, const char* str);
void	DYN_error_punt(bool, USHORT);
void	DYN_execute(Jrd::Global*, const UCHAR**, const Firebird::MetaName*, Firebird::MetaName*, Firebird::MetaName*, Firebird::MetaName*, Firebird::MetaName*);
SLONG	DYN_get_number(const UCHAR**);
USHORT	DYN_get_string(const TEXT**, Firebird::MetaName&, size_t, bool);
USHORT	DYN_get_string(const TEXT**, Firebird::PathName&, size_t, bool);
USHORT	DYN_get_string(const TEXT**, Firebird::UCharBuffer&, size_t, bool);
USHORT	DYN_get_string(const TEXT**, TEXT*, size_t, bool);

bool	DYN_is_it_sql_role(Jrd::Global*, const Firebird::MetaName&, Firebird::MetaName&, Jrd::thread_db*);
USHORT	DYN_put_blr_blob(Jrd::Global*, const UCHAR**, Jrd::bid*);
USHORT	DYN_put_text_blob(Jrd::Global*, const UCHAR**, Jrd::bid*);

void	DYN_rundown_request(Jrd::jrd_req*, SSHORT);
USHORT	DYN_skip_attribute(const UCHAR**);
USHORT	DYN_skip_blr_blob(const UCHAR**);

// This function is not defined anywhere.
//USHORT	DYN_skip_attribute2(const UCHAR**);

void	DYN_unsupported_verb(void);

#endif // JRD_DYN_H

