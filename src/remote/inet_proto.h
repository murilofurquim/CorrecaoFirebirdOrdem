/*
 *	PROGRAM:	JRD Remote Interface/Server
 *	MODULE:		inet_proto.h
 *	DESCRIPTION:	Prototpe header file for inet.cpp
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

#ifndef REMOTE_INET_PROTO_H
#define REMOTE_INET_PROTO_H

#include "../common/classes/fb_string.h"

rem_port*	INET_analyze(Firebird::PathName&, ISC_STATUS*, const TEXT*, const TEXT*,
	bool, const UCHAR*, USHORT);
rem_port*	INET_connect(const TEXT*, struct packet*, ISC_STATUS*, USHORT,
	const UCHAR*, USHORT);
rem_port*	INET_reconnect(HANDLE, ISC_STATUS*);
rem_port*	INET_server(int);
void	INET_set_clients(int);

#endif // REMOTE_INET_PROTO_H

