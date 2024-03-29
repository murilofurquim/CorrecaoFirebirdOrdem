/*
 *	PROGRAM:	InterBase International support
 *	MODULE:		lc_unicode_ucs2.cpp
 *	DESCRIPTION:	Language Drivers in the Unicode family.
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

#include "firebird.h"
#include "../intl/ldcommon.h"
#include "lc_ascii.h"
#include "cv_unicode_fss.h"
#include "ld_proto.h"

static inline bool FAMILY_UNICODE_WIDE_BIN(texttype* cache,
										   SSHORT country,
										   const ASCII* POSIX,
										   USHORT attributes,
										   const UCHAR* specific_attributes,
										   ULONG specific_attributes_length)
//#define FAMILY_UNICODE_WIDE_BIN(id_number, name, charset, country)
{
	if ((attributes & ~TEXTTYPE_ATTR_PAD_SPACE) || specific_attributes_length)
		return false;

	cache->texttype_version			= TEXTTYPE_VERSION_1;
	cache->texttype_name			= POSIX;
	cache->texttype_country			= country;
	cache->texttype_pad_option		= (attributes & TEXTTYPE_ATTR_PAD_SPACE) ? true : false;
	cache->texttype_fn_key_length	= famasc_key_length;
	cache->texttype_fn_string_to_key= famasc_string_to_key;
	cache->texttype_fn_compare		= famasc_compare;
	//cache->texttype_fn_str_to_upper = famasc_str_to_upper;
	//cache->texttype_fn_str_to_lower = famasc_str_to_lower;

	return true;
}

static inline bool FAMILY_UNICODE_MB_BIN(texttype* cache,
										 SSHORT country,
										 const ASCII* POSIX,
										 USHORT attributes,
										 const UCHAR* specific_attributes,
										 ULONG specific_attributes_length)
//#define FAMILY_UNICODE_MB_BIN(id_number, name, charset, country)
{
	if ((attributes & ~TEXTTYPE_ATTR_PAD_SPACE) || specific_attributes_length)
		return false;

	cache->texttype_version			= TEXTTYPE_VERSION_1;
	cache->texttype_name			= POSIX;
	cache->texttype_country			= country;
	cache->texttype_pad_option		= (attributes & TEXTTYPE_ATTR_PAD_SPACE) ? true : false;
	cache->texttype_fn_key_length	= famasc_key_length;
	cache->texttype_fn_string_to_key= famasc_string_to_key;
	cache->texttype_fn_compare		= famasc_compare;
	//cache->texttype_fn_str_to_upper = famasc_str_to_upper;
	//cache->texttype_fn_str_to_lower = famasc_str_to_lower;

	return true;
}


TEXTTYPE_ENTRY(UNI200_init)
{
	static ASCII POSIX[] = "C.UNICODE";

	return FAMILY_UNICODE_WIDE_BIN(cache, CC_C, POSIX, attributes, specific_attributes, specific_attributes_length);
}


TEXTTYPE_ENTRY(UNI201_init)
{
	static ASCII POSIX[] = "C.UNICODE_FSS";

	return FAMILY_UNICODE_MB_BIN(cache, CC_C, POSIX, attributes, specific_attributes, specific_attributes_length);
}
