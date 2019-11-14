/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		tra.h
 *	DESCRIPTION:	Transaction block definitions
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
 * 2001.6.25 Claudio Valderrama: add dfw_delete_generator and dfw_delete_udf
 *           to the dfw_t enumeration.
 * 2002.10.29 Nickolay Samofatov: Added support for savepoints
 */

#ifndef JRD_TRA_H
#define JRD_TRA_H

/*
 * TMN: Fix this header! It should include any header it needs
 * to define the types this header uses.
 */

#include "../jrd/jrd_blks.h"
#include "../include/fb_blk.h"
#include "../common/classes/tree.h"
#include "../common/classes/GenericMap.h"
#include "../jrd/rpb_chain.h"
#include "../jrd/exe.h"
#include "../jrd/blb.h" // For bid structure
#include "../jrd/sbm.h" // For bid structure

#include "../jrd/DatabaseSnapshot.h"
#include "../jrd/TempSpace.h"

namespace Jrd {

class blb;
class Lock;
class jrd_rel;
template <typename T> class vec;
class Savepoint;
class Record;
class VerbAction;
class ArrayField;
class Attachment;
class DeferredJob;

const SLONG MAX_TRA_NUMBER = MAX_SLONG;

// Blobs active in transaction identified by bli_temp_id. Please keep this 
// structure small as there can be huge amount of them floating in memory.
struct BlobIndex {
	ULONG bli_temp_id;
	bool bli_materialized;
	jrd_req* bli_request;
	union {
		bid bli_blob_id; // ID of materialized blob
		blb* bli_blob_object; // Blob object
	};
    static const ULONG& generate(const void *sender, const BlobIndex& item) {
		return item.bli_temp_id;
    }
	// Empty default constructor to make it behave like POD structure
	BlobIndex() {}
	BlobIndex(ULONG temp_id, blb* blob_object) : 
		bli_temp_id(temp_id), bli_materialized(false), bli_request(NULL), 
		bli_blob_object(blob_object) 
	{ }
};

typedef Firebird::BePlusTree<BlobIndex, ULONG, MemoryPool, BlobIndex> BlobIndexTree;

/* Transaction block */

const int DEFAULT_LOCK_TIMEOUT = -1; // infinite
const char* const TRA_BLOB_SPACE = "fb_blob_";

class jrd_tra : public pool_alloc_rpt<SCHAR, type_tra>
{
public:
	enum wait_t {
		tra_no_wait,
		tra_probe,
		tra_wait
	};

	jrd_tra(JrdMemoryPool* p) :
		tra_pool(p),
		tra_blobs(p),
		tra_deferred_job(NULL),
		tra_resources(*p),
		tra_context_vars(*p),
		tra_lock_timeout(DEFAULT_LOCK_TIMEOUT),
		tra_blob_space(NULL)
	{}

	~jrd_tra()
	{
		delete tra_blob_space;
	}

	static void destroy(Database* const dbb, jrd_tra* const transaction)
	{
		if (transaction)
		{
			JrdMemoryPool* const pool = transaction->tra_pool;
			delete transaction;

			if (pool) 
				JrdMemoryPool::deletePool(pool);
		}
	}

	Attachment* tra_attachment;	/* database attachment */
	SLONG tra_number;			/* transaction number */
	SLONG tra_top;				/* highest transaction in snapshot */
	SLONG tra_oldest;			/* oldest interesting transaction */
	SLONG tra_oldest_active;	/* record versions older than this can be
								   gargage-collected by this tx */
	jrd_tra*	tra_next;		/* next transaction in database */
	jrd_tra*	tra_sibling;	/* next transaction in group */
	JrdMemoryPool* tra_pool;	/* pool for transaction */
	BlobIndexTree tra_blobs;		/* list of active blobs */
	ArrayField*	tra_arrays;		/* Linked list of active arrays */
	Lock*		tra_lock;		/* lock for transaction */
	Lock*		tra_cancel_lock;	/* lock to cancel the active request */
	vec<Lock*>*		tra_relation_locks;	/* locks for relations */
	UInt32Bitmap*	tra_commit_sub_trans;	/* commited sub-transactions */
	Savepoint*	tra_save_point;	/* list of savepoints */
	Savepoint*	tra_save_free;	/* free savepoints */
	SLONG tra_save_point_number;	/* next save point number to use */
	ULONG tra_flags;
	DeferredJob*	tra_deferred_job;	/* work deferred to commit time */
	ResourceList tra_resources;		/* resource existence list */
	Firebird::StringMap tra_context_vars; // Context variables for the transaction
	traRpbList* tra_rpblist;	/* active record_param's of given transaction */
	UCHAR tra_use_count;		/* use count for safe AST delivery */
	UCHAR tra_callback_count;	/* callback count for 'execute statement' */
	SSHORT tra_lock_timeout;	/* in seconds, -1 means infinite, 0 means NOWAIT */
	ULONG tra_next_blob_id;     // ID of the previous blob or array created in this transaction
	Firebird::TimeStamp tra_timestamp; // transaction start time
	jrd_req* tra_requests;		// Doubly linked list of requests active in this transaction
	DatabaseSnapshot* tra_db_snapshot; // Database state snapshot (for monitoring purposes)
	RuntimeStatistics tra_stats;

private:
	TempSpace* tra_blob_space;	// temp space storage

public:
	SSHORT getLockWait() const
	{
		return -tra_lock_timeout;
	}

	TempSpace* getTempSpace()
	{
		if (!tra_blob_space)
			tra_blob_space = FB_NEW(*tra_pool) TempSpace(*tra_pool, TRA_BLOB_SPACE);

		return tra_blob_space;
	}

public:
	UCHAR tra_transactions[1];
};

// System transaction is always transaction 0.
const SLONG TRA_system_transaction = 0;

// Flag definitions for tra_flags.

const ULONG TRA_system			= 1L;		/* system transaction */
//const ULONG TRA_update			= 2L;		// update is permitted 
const ULONG TRA_prepared		= 4L;		/* transaction is in limbo */
const ULONG TRA_reconnected		= 8L;		/* reconnect in progress */
// How can RLCK_reserve_relation test for it if it's not set anywhere?
//const ULONG TRA_reserving		= 16L;		// relations explicityly locked
const ULONG TRA_degree3			= 32L;		/* serializeable transaction */
//const ULONG TRA_committing		= 64L;		// commit in progress
const ULONG TRA_write			= 128L;		/* transaction has written */
const ULONG TRA_readonly		= 256L;		/* transaction is readonly */
//const ULONG TRA_nowait			= 512L;		// don't wait on relations, give up
const ULONG TRA_prepare2		= 1024L;	/* transaction has updated RDB$TRANSACTIONS */
const ULONG TRA_ignore_limbo	= 2048L;	/* ignore transactions in limbo */
const ULONG TRA_invalidated 	= 4096L;	/* transaction invalidated by failed write */
const ULONG TRA_deferred_meta 	= 8192L;	/* deferred meta work posted */
//const ULONG TRA_add_log		= 16384L;	// write ahead log file was added
//const ULONG TRA_delete_log	= 32768L;	// write ahead log file was deleted
const ULONG TRA_read_committed	= 65536L;	/* can see latest committed records */
const ULONG TRA_autocommit		= 131072L;	/* autocommits all updates */
const ULONG TRA_perform_autocommit	= 262144L;	/* indicates autocommit is necessary */
const ULONG TRA_rec_version			= 524288L;	/* don't wait for uncommitted versions */
const ULONG TRA_restart_requests	= 1048576L;	/* restart all requests in attachment */
const ULONG TRA_no_auto_undo		= 2097152L;	/* don't start a savepoint in TRA_start */
const ULONG TRA_cancel_request		= 4194304L;	// cancel active request, if any
const ULONG TRA_precommitted		= 8388608L;	/* transaction committed at startup */

const int TRA_MASK				= 3;
//const int TRA_BITS_PER_TRANS	= 2;
//const int TRA_TRANS_PER_BYTE	= 4;
const int TRA_SHIFT				= 2;

#define TRANS_SHIFT(number)	(((number) & TRA_MASK) << 1)
#define TRANS_OFFSET(number)	((number) >> TRA_SHIFT)

/* Transaction cleanup. If a database is never quiescent, look
   for "dead" active transactions every so often at transaction
   startup */

const int TRA_ACTIVE_CLEANUP	= 100;

/* Transaction states.  The first four are states found
   in the transaction inventory page; the last two are
   returned internally */

const int tra_active		= 0;	/* Transaction is active */
const int tra_limbo			= 1;
const int tra_dead			= 2;
const int tra_committed		= 3;
const int tra_us			= 4;	/* Transaction is us */
const int tra_precommitted	= 5;	/* Transaction is precommitted */

// The highest transaction number possible.  This is 0x7fffffff if SLONG is 32 bits.
//#define MAX_TRA_NUMBER		 (~(1L << (BITS_PER_LONG - 1)))

/* Savepoint block */

class Savepoint : public pool_alloc<type_sav>
{
public:
	~Savepoint()
	{
		deleteActions(sav_verb_actions);
		deleteActions(sav_verb_free);
	}

	VerbAction*		sav_verb_actions;	/* verb action list */
	VerbAction*		sav_verb_free;		/* free verb actions */
	USHORT			sav_verb_count;		/* Active verb count */
	SLONG			sav_number;			/* save point number */
	Savepoint*		sav_next;
	USHORT			sav_flags;
	TEXT			sav_name[32]; /* Savepoint name */

private:
	void deleteActions(VerbAction* list);
};

/* Savepoint block flags. */

const int SAV_trans_level	= 1;	/* savepoint was started by TRA_start */
const int SAV_event_post	= 2;	/* event posted in the save point */
const int SAV_user			= 4;	/* named user savepoint as opposed to system ones */

/* Maximum size in bytes of transaction-level savepoint data.
   When transaction-level savepoint gets past this size we drop it and use GC
   mechanisms to clean out changes done in transaction */
const IPTR SAV_LARGE		= 1024 * 32;

/* Deferred work blocks are used by the meta data handler to keep track
   of work deferred to commit time.  This are usually used to perform
   meta data updates */

enum dfw_t {
	dfw_null,
	dfw_create_relation,
	dfw_delete_relation,
	dfw_update_format,
	dfw_create_index,
	dfw_delete_index,
	dfw_compute_security,
	dfw_add_file,
	dfw_add_shadow,
	dfw_delete_shadow,
	dfw_modify_file,
	dfw_erase_file,
	dfw_create_field,
	dfw_delete_field,
	dfw_modify_field,
	dfw_delete_global,
	dfw_delete_rfr,
	dfw_post_event,
	dfw_create_trigger,
	dfw_delete_trigger,
	dfw_modify_trigger,
	//dfw_load_triggers,
	dfw_grant,
	dfw_revoke,
	dfw_scan_relation,
	dfw_create_expression_index,
	dfw_delete_expression_index,
	dfw_create_procedure,
	dfw_modify_procedure,
	dfw_delete_procedure,
	dfw_delete_prm, 
	dfw_create_collation,
	dfw_delete_collation,
	dfw_delete_exception, 
	//dfw_unlink_file,
	dfw_delete_generator,
	dfw_modify_generator,
	dfw_delete_udf,
	dfw_add_difference,
	dfw_delete_difference,
	dfw_begin_backup,
	dfw_end_backup,

	// deferred works argument types
	dfw_arg_index_name,		// index name for dfw_delete_expression_index, mandatory
	dfw_arg_partner_rel_id,	// partner relation id for dfw_delete_index if index is FK, optional
	dfw_arg_proc_name,		// procedure name for dfw_delete_prm, mandatory
	dfw_arg_force_computed,	// we need to drop dependencies from a field that WAS computed
	dfw_arg_check_blr,		// check if BLR is still compilable
	dfw_arg_rel_name,		// relation name of a trigger
	dfw_arg_trg_type		// trigger type
};

/* Verb actions */

class UndoItem {
public:
	SINT64 rec_number;
	Record* rec_data;
    static const SINT64& generate(const void *sender, const UndoItem& item) {
		return item.rec_number;
    }
	UndoItem() {
	}
	UndoItem(SINT64 rec_numberL, Record* rec_dataL) {
		this->rec_number = rec_numberL;
		this->rec_data = rec_dataL;
	}
};

typedef Firebird::BePlusTree<UndoItem, SINT64, MemoryPool, UndoItem> UndoItemTree;

class VerbAction : public pool_alloc<type_vct>
{
public:
	~VerbAction()
	{
		delete vct_records;
		delete vct_undo;
	}

	VerbAction* 	vct_next;		/* Next action within verb */
	jrd_rel*		vct_relation;	/* Relation involved */
	RecordBitmap*	vct_records;	/* Record involved */
	UndoItemTree*	vct_undo;		/* Data for undo records */
};


inline void Savepoint::deleteActions(VerbAction* list)
{
	while (list)
	{
		VerbAction* next = list->vct_next;
		delete list;
		list = next;
	}
};

} //namespace Jrd

#endif // JRD_TRA_H

