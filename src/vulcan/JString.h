/*
 *  
 *     The contents of this file are subject to the Initial 
 *     Developer's Public License Version 1.0 (the "License"); 
 *     you may not use this file except in compliance with the 
 *     License. You may obtain a copy of the License at 
 *     http://www.ibphoenix.com/idpl.html. 
 *
 *     Software distributed under the License is distributed on 
 *     an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either 
 *     express or implied.  See the License for the specific 
 *     language governing rights and limitations under the License.
 *
 *     The contents of this file or any work derived from this file
 *     may not be distributed under any other license whatsoever 
 *     without the express prior written permission of the original 
 *     author.
 *
 *
 *  The Original Code was created by James A. Starkey for IBPhoenix.
 *
 *  Copyright (c) 1997 - 2000, 2001, 2003 James A. Starkey
 *  Copyright (c) 1997 - 2000, 2001, 2003 Netfrastructure, Inc.
 *  All Rights Reserved.
 */


#ifndef __JString_H
#define __JString_H


START_NAMESPACE

class JString 
{
public:
	JString (const char *source, int length);
	JString();
	JString (const char *string);
	JString(const JString& stringSrc);
	~JString();

	//JString&	operator = (const WCHAR *wString);
	//JString (const WCHAR *wString);
	//JString (const WCHAR *wString, int len);

	void		releaseBuffer ();
	char*		getBuffer (int length);
	int			length() const;
	bool		equalsNoCase (const char *string2) const;
	static		JString upcase (const char *source);
	static int	findSubstring (const char *string, const char *sub);
	int			hash (int tableSize) const;
	static int	hash (const char *string, int tableSize);
	bool		IsEmpty() const;
	const char* after (char c) const;
	JString		before (char c) const;
	void		append (const char*);
	void		setString (const char*);
	void		setString (const char *source, int length);
	void		Format (const char*, ...);
	
	inline const char	*getString() const
		{
		return (string) ? string : "";
		}

	inline const char	*c_str() const
		{
		return getString();
		}

	inline 	operator const char*() const
		{
		return getString();
		}
	
	//void set (int length, const char *stuff);
	//void		setString (const WCHAR *wString, int len);
	
	bool		operator == (const char *string) const;
	bool		operator != (const char *stuff) const;
	//JString		operator + (const char *string);
				operator bool() const;
	bool		operator !() const;
	JString&	operator = (const char *string);
	JString&	operator = (const JString& string);
	JString&	operator+=(const char *string);
	JString&	operator+=(const JString& string);

	friend JString operator + (const JString& string1, const char* string2);

protected:
	void		alloc (int length);
	void		release();

	inline void	release(char *stuff)
		{
		stuff -= sizeof (int);
		if (--((int*) stuff)[0] == 0)
			delete [] stuff;
		}
	
	inline void allocSpace(int len)
		{
		string = new char [len + 1 + sizeof (int)];
		*((int*) string) = 1;
		string += sizeof (int);
		}
			
	char	*string;
};

END_NAMESPACE

#endif
