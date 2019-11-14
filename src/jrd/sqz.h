/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		sqz.h
 *	DESCRIPTION:	Data compression control block
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

#ifndef JRD_SQZ_H
#define JRD_SQZ_H

#include "../jrd/all.h"
#include "../include/fb_blk.h"

namespace Jrd {

class DataComprControl : public pool_alloc<type_dcc>
{
    public:
	JrdMemoryPool* dcc_pool;
	DataComprControl* dcc_next;	// Next block if overflow
	const SCHAR* dcc_end;		// End of control string
	SCHAR dcc_string[128];
};

} //namespace Jrd

#endif // JRD_SQZ_H

