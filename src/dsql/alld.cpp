/*
 *	PROGRAM:	Dynamic SQL runtime support
 *	MODULE:		all.cpp
 *	DESCRIPTION:	Internal block allocator
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
 *	02 Nov 2001: Mike Nordell - Synch with FB1 changes.
 */

/**************************************************************
V4 Multi-threading changes.

-- direct calls to gds__ () & isc_ () entry points

	THREAD_EXIT();
	    gds__ () or isc_ () call.
	THREAD_ENTER();

-- calls through embedded GDML.

the following protocol will be used.  Care should be taken if
nested FOR loops are added.

    THREAD_EXIT();                // last statment before FOR loop 

    FOR ...............

	THREAD_ENTER();           // First statment in FOR loop
	.....some C code....
	.....some C code....
	THREAD_EXIT();            // last statment in FOR loop 

    END_FOR;

    THREAD_ENTER();               // First statment after FOR loop
***************************************************************/

#include "firebird.h"
#include <string.h>
#include <stdio.h>
#include "../dsql/all.h"
#include "../dsql/dsql.h"
#include "../dsql/alld_proto.h"
#include "../dsql/errd_proto.h"
#include "../jrd/gds_proto.h"
#include "../jrd/thd.h"
#include "../jrd/thread_proto.h"

#include "../common/classes/array.h"

DsqlMemoryPool* DSQL_permanent_pool = 0;
typedef Firebird::Array<DsqlMemoryPool*> pool_vec_t;
static bool init_flag = false;
static pool_vec_t *pools = 0;

// Microsoft MSVC bug workaround
#ifdef _MSC_VER
#define for if(0) {} else for
#endif


void ALLD_fini()
{
	if (!init_flag) {
		ERRD_bugcheck("ALLD_fini - finishing before starting");
	}

	for (pool_vec_t::iterator curr = pools->begin(); curr != pools->end(); ++curr)
	{
		if (*curr) {
			DsqlMemoryPool::deletePool(*curr);
		}
	}

	delete pools;
	pools = 0;
	DsqlMemoryPool::deletePool(DSQL_permanent_pool);
	DSQL_permanent_pool = 0;
	init_flag = false;
}


void ALLD_init()
{
	if (!init_flag)
	{
		init_flag = true;
		DSQL_permanent_pool = DsqlMemoryPool::createPool();
		pools = FB_NEW(*DSQL_permanent_pool) pool_vec_t(*DSQL_permanent_pool, 10);
	}
}


DsqlMemoryPool* DsqlMemoryPool::createPool()
{
	DsqlMemoryPool* result = (DsqlMemoryPool*)internal_create(sizeof(DsqlMemoryPool));
	
	if (!DSQL_permanent_pool)
		return result;
		
	for (pool_vec_t::iterator curr = pools->begin(); curr != pools->end(); ++curr)
	{
		if (!*curr)
		{
			*curr = result;
			return result;
		}
	}

	pools->resize(pools->getCount() + 10);
	for (pool_vec_t::iterator curr = pools->begin(); curr != pools->end(); ++curr)
	{
		if (!*curr)
		{
			*curr = result;
			return result;
		}
	}

	ERRD_bugcheck("ALLD_fini - finishing before starting");
	return NULL; //silencer
}

void DsqlMemoryPool::deletePool(DsqlMemoryPool* pool)
{
	MemoryPool::deletePool(pool);
	
	if (pool == DSQL_permanent_pool)
		return;
		
	for (pool_vec_t::iterator curr = pools->begin(); curr != pools->end(); ++curr)
	{
		if (*curr == pool)
		{
			*curr = 0;
			return;
		}
	}
}

