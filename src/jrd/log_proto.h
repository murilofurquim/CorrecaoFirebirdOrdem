/*
 *	PROGRAM:	JRD Access method
 *	MODULE:		log_proto.h		
 *	DESCRIPTION:	Prototype Header file for log.cpp
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

#ifndef JRD_LOG_PROTO_H
#define JRD_LOG_PROTO_H

#ifdef REPLAY_OSRI_API_CALLS_SUBSYSTEM

void	LOG_call(enum log_t, ...);
void	LOG_disable(void);
void	LOG_enable(const TEXT*, USHORT);
void	LOG_fini(void);
void	LOG_init(const TEXT*, USHORT);

#endif /* REPLAY_OSRI_API_CALLS_SUBSYSTEM */

#endif // JRD_LOG_PROTO_H
