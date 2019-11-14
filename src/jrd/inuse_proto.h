/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		inuse_proto.h
 *	DESCRIPTION:	Prototype header file for inuse.cpp
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

#ifndef JRD_INUSE_PROTO_H
#define JRD_INUSE_PROTO_H

bool INUSE_cleanup(struct iuo*, FPTR_VOID_PTR);
void INUSE_clear(struct iuo*);
bool INUSE_insert(struct iuo*, void*, bool);
bool INUSE_remove(struct iuo*, void*, bool);

#endif // JRD_INUSE_PROTO_H

