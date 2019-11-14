/*
 *	PROGRAM:	JRD Command Oriented Query Language
 *	MODULE:		parse_proto.h
 *	DESCRIPTION:	Prototype header file for parse.cpp
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

#ifndef QLI_PARSE_PROTO_H
#define QLI_PARSE_PROTO_H

qli_syntax*	PARQ_parse(void);
bool	PAR_match (KWWORDS);
void	PAR_real (void);
void	PAR_real_token (void);
void	PAR_token (void);

#endif // QLI_PARSE_PROTO_H

