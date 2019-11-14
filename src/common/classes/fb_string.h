/*
 *	PROGRAM:	string class definition
 *	MODULE:		fb_string.h
 *	DESCRIPTION:	Provides almost that same functionality,
 *			that STL::basic_string<char> does, 
 *			but behaves MemoryPools friendly.
 *
 *  The contents of this file are subject to the Initial
 *  Developer's Public License Version 1.0 (the "License");
 *  you may not use this file except in compliance with the
 *  License. You may obtain a copy of the License at
 *  http://www.ibphoenix.com/main.nfs?a=ibphoenix&page=ibp_idpl.
 *
 *  Software distributed under the License is distributed AS IS,
 *  WITHOUT WARRANTY OF ANY KIND, either express or implied.
 *  See the License for the specific language governing rights
 *  and limitations under the License.
 *
 *  The Original Code was created by Alexander Peshkoff
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2004 Alexander Peshkoff <peshkoff@mail.ru>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 */

#ifndef INCLUDE_FB_STRING_H
#define INCLUDE_FB_STRING_H

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "firebird.h"
#include "../include/fb_types.h"
#include "../include/fb_exception.h"
#include "../common/classes/alloc.h"

namespace Firebird
{
	class AbstractString : private AutoStorage {
	public:
		typedef char char_type;
		typedef size_t size_type;
		typedef ptrdiff_t difference_type;
		typedef char* pointer;
		typedef const char* const_pointer;
		typedef char& reference;
		typedef const char& const_reference;
		typedef char value_type;
		typedef pointer iterator;
		typedef const_pointer const_iterator;
		static const size_type npos;
		enum {INLINE_BUFFER_SIZE = 32, INIT_RESERVE = 16/*, KEEP_SIZE = 512*/};
		
	protected:
		typedef USHORT internal_size_type; // 16 bits!
		char_type inlineBuffer[INLINE_BUFFER_SIZE];
		char_type* stringBuffer;
		internal_size_type stringLength, bufferSize;
		
	private:
		inline void checkPos(size_type pos) const {
			if (pos >= length()) {
				fatal_exception::raise("Firebird::string - pos out of range");
			}
		}
		
		static inline void checkLength(size_type len) {
			if (len > max_length()) {
				fatal_exception::raise("Firebird::string - length exceeds predefined limit");
			}
		}

		// Reserve buffer to allow storing at least newLen characters there
		// (not including null terminator). Existing contents of our string are preserved.
		void reserveBuffer(size_type newLen) {
			size_type newSize = newLen + 1;
			if (newSize > bufferSize) {
				// Make sure we do not exceed string length limit
				checkLength(newLen);

				// Order of assignments below is important in case of low memory conditions

				// Grow buffer exponentially to prevent memory fragmentation
				if (newSize / 2 < bufferSize)
					newSize = bufferSize * 2;

				// Do not grow buffer beyond string length limit
				if (newSize > max_length() + 1)
					newSize = max_length() + 1;

				// Allocate new buffer
				char_type *newBuffer = FB_NEW(getPool()) char_type[newSize];

				// Carefully copy string data including null terminator
				memcpy(newBuffer, stringBuffer, sizeof(char_type) * (stringLength + 1));

				// Deallocate old buffer if needed
				if (stringBuffer != inlineBuffer)
					delete[] stringBuffer;

				stringBuffer = newBuffer;
				bufferSize = static_cast<internal_size_type>(newSize);
			}
		}

		// Make sure our buffer is large enough to store at least <length> characters in it
		// (not including null terminator). Resulting buffer is not initialized. 
		// Use it in constructors only when stringBuffer is not assigned yet.
		void initialize(size_type len) {
			if (len < INLINE_BUFFER_SIZE) {
				stringBuffer = inlineBuffer;
				bufferSize = INLINE_BUFFER_SIZE;
			} 
			else {				
				stringBuffer = NULL; // Be safe in case of exception
				checkLength(len);

				// Reserve a few extra bytes in the buffer
				size_type newSize = len + 1 + INIT_RESERVE;

				// Do not grow buffer beyond string length limit
				if (newSize > max_length() + 1)
					newSize = max_length() + 1;

				// Allocate new buffer
				stringBuffer = FB_NEW(getPool()) char_type[newSize];
				bufferSize = static_cast<internal_size_type>(newSize);
			}
			stringLength = static_cast<internal_size_type>(len);
			stringBuffer[stringLength] = 0;
		}

		void shrinkBuffer() {
			// Shrink buffer if we decide it is beneficial
		}

	protected:
		AbstractString(size_type sizeL, const_pointer datap);

		AbstractString(const_pointer p1, size_type n1, 
					 const_pointer p2, size_type n2);

		AbstractString(const AbstractString& v);

		inline AbstractString() : 
			stringBuffer(inlineBuffer), stringLength(0), bufferSize(INLINE_BUFFER_SIZE)
		{
			stringBuffer[0] = 0;
		}

		AbstractString(size_type sizeL, char_type c);

		inline explicit AbstractString(MemoryPool& p) : AutoStorage(p),
			stringBuffer(inlineBuffer), stringLength(0), bufferSize(INLINE_BUFFER_SIZE)
		{
			stringBuffer[0] = 0;
		}

		inline AbstractString(MemoryPool& p, const AbstractString& v) 
			: AutoStorage(p)
		{
			initialize(v.length());
			memcpy(stringBuffer, v.c_str(), stringLength);
		}

		inline AbstractString(MemoryPool& p, const char_type* s, size_type l) 
			: AutoStorage(p)
		{
			initialize(l);
			memcpy(stringBuffer, s, l);
		}

		pointer Modify(void) {
			return stringBuffer;
		}

		// Trim the range making sure that it fits inside specified length
		static void AdjustRange(size_type length, size_type& pos, size_type& n);

		pointer baseAssign(size_type n);

		pointer baseAppend(size_type n);

		pointer baseInsert(size_type p0, size_type n);

		void baseErase(size_type p0, size_type n);

		enum TrimType {TrimLeft, TrimRight, TrimBoth};

		void baseTrim(TrimType WhereTrim, const_pointer ToTrim);

	public:
		inline const_pointer c_str() const {
			return stringBuffer;
		}
		inline size_type length() const {
			return stringLength;
		}
		// Almost same as c_str(), but return 0, not "",
		// when string has no data. Useful when interacting
		// with old code, which does check for NULL.
		inline const_pointer nullStr() const {
			return stringLength ? stringBuffer : 0;
		}
		// Call it only when you have worked with at() or operator[]
		// in case a null ASCII was inserted in the middle of the string.
		inline size_type recalculate_length()
		{
		    stringLength = static_cast<internal_size_type>(strlen(stringBuffer));
		    return stringLength;
		}

		void reserve(size_type n = 0);
		void resize(size_type n, char_type c = ' ');

		inline pointer getBuffer(size_t l)
		{
			return baseAssign(l);
		}

/*		inline void swap(AbstractString& str) {
			Storage *tmp = StringData;
			StringData = str.StringData;
			str.StringData = tmp;
		}*/

		inline size_type find(const AbstractString& str, size_type pos = 0) const {
			return find(str.c_str(), pos);
		}
		inline size_type find(const_pointer s, size_type pos = 0) const {
			const_pointer p = strstr(c_str() + pos, s);
			return p ? p - c_str() : npos;
		}
		inline size_type find(char_type c, size_type pos = 0) const {
			const_pointer p = strchr(c_str() + pos, c);
			return p ? p - c_str() : npos;
		}
		inline size_type rfind(const AbstractString& str, size_type pos = npos) const {
			return rfind(str.c_str(), pos);
		}
		size_type rfind(const_pointer s, size_type pos = npos) const;
		size_type rfind(char_type c, size_type pos = npos) const;
		inline size_type find_first_of(const AbstractString& str, size_type pos = 0) const {
			return find_first_of(str.c_str(), pos, str.length());
		}
		size_type find_first_of(const_pointer s, size_type pos, size_type n) const;
		inline size_type find_first_of(const_pointer s, size_type pos = 0) const {
			return find_first_of(s, pos, strlen(s));
		}
		inline size_type find_first_of(char_type c, size_type pos = 0) const {
			return find(c, pos);
		}
		inline size_type find_last_of(const AbstractString& str, size_type pos = npos) const {
			return find_last_of(str.c_str(), pos, str.length());
		}
		size_type find_last_of(const_pointer s, size_type pos, size_type n = npos) const;
		inline size_type find_last_of(const_pointer s, size_type pos = npos) const {
			return find_last_of(s, pos, strlen(s));
		}
		inline size_type find_last_of(char_type c, size_type pos = npos) const {
			return rfind(c, pos);
		}
		inline size_type find_first_not_of(const AbstractString& str, size_type pos = 0) const {
			return find_first_not_of(str.c_str(), pos, str.length());
		}
		size_type find_first_not_of(const_pointer s, size_type pos, size_type n) const;
		inline size_type find_first_not_of(const_pointer s, size_type pos = 0) const {
			return find_first_not_of(s, pos, strlen(s));
		}
		inline size_type find_first_not_of(char_type c, size_type pos = 0) const {
			const char s[2] = {c, 0};
			return find_first_not_of(s, pos, 1);
		}
		inline size_type find_last_not_of(const AbstractString& str, size_type pos = npos) const {
			return find_last_not_of(str.c_str(), pos, str.length());
		}
		size_type find_last_not_of(const_pointer s, size_type pos, size_type n = npos) const;
		inline size_type find_last_not_of(const_pointer s, size_type pos = npos) const {
			return find_last_not_of(s, pos, strlen(s));
		}
		inline size_type find_last_not_of(char_type c, size_type pos = npos) const {
			const char s[2] = {c, 0};
			return find_last_not_of(s, pos, 1);
		}

		inline iterator begin() {
			return Modify();
		}
		inline const_iterator begin() const {
			return c_str();
		}
		inline iterator end() {
			return Modify() + length();
		}
		inline const_iterator end() const {
			return c_str() + length();
		}
		inline const_reference at(size_type pos) const {
			checkPos(pos);
			return c_str()[pos];
		}
		inline reference at(size_type pos) {
			checkPos(pos);
			return Modify()[pos];
		}
		inline const_reference operator[](size_type pos) const {
			return at(pos);
		}
		inline reference operator[](size_type pos) {
			return at(pos);
		}
		inline const_pointer data() const {
			return c_str();
		}
		inline size_type size() const {
			return length();
		}
		static inline size_type max_length() {
			return 0xfffe; // Max length of character field in Firebird
		}
		inline size_type capacity() const {
			return bufferSize - 1;
		}
		inline bool empty() const {
			return length() == 0;
		}
		inline bool hasData() const {
			return !empty();
		}
		// to satisfy both ways to check for empty string
		inline bool isEmpty() const {
			return empty();
		}

		void upper();
		void lower();
		inline void ltrim(const_pointer ToTrim = " ") {
			baseTrim(TrimLeft, ToTrim);
		}
		inline void rtrim(const_pointer ToTrim = " ") {
			baseTrim(TrimRight, ToTrim);
		}
		inline void trim(const_pointer ToTrim = " ") {
			baseTrim(TrimBoth, ToTrim);
		}
		inline void alltrim(const_pointer ToTrim = " ") {
			baseTrim(TrimBoth, ToTrim);
		}

		bool LoadFromFile(FILE *file);
		void vprintf(const char* Format, va_list params);
		void printf(const char* Format, ...);

		inline size_type copyTo(pointer to, size_type toSize) const
		{
			fb_assert(to);
			fb_assert(toSize);
			if (--toSize > length())
			{
				toSize = length();
			}
			memcpy(to, c_str(), toSize);
			to[toSize] = 0;
			return toSize;
		}

		void appendQuoted(const AbstractString& from, char_type quote);

		inline ~AbstractString() {
			if (stringBuffer != inlineBuffer)
				delete[] stringBuffer;
		}
	};

	class StringComparator {
	public:
		static inline int compare(AbstractString::const_pointer s1, AbstractString::const_pointer s2, AbstractString::size_type n) {
			return memcmp(s1, s2, n);
		}
	};
	class PathNameComparator {
	public:
		static int compare(AbstractString::const_pointer s1, AbstractString::const_pointer s2, AbstractString::size_type n);
	};

	template<typename Comparator>
	class StringBase : public AbstractString {
		typedef StringBase<Comparator> StringType;
	protected:
		inline StringBase<Comparator>(const_pointer p1, size_type n1, 
						  const_pointer p2, size_type n2) :
			   AbstractString(p1, n1, p2, n2) {}
	private:
		inline StringType add(const_pointer s, size_type n) const {
			return StringBase<Comparator>(c_str(), length(), s, n);
		}
	public:
		inline StringBase<Comparator>() : AbstractString() {}
		inline StringBase<Comparator>(const StringType& v) : AbstractString(v) {}
		inline StringBase<Comparator>(const_pointer s, size_type n) : AbstractString(n, s) {}
		inline StringBase<Comparator>(const_pointer s) : AbstractString(strlen(s), s) {}
		inline StringBase<Comparator>(const unsigned char* s) : AbstractString(strlen((char*)s), (char*)s) {}
		inline StringBase<Comparator>(size_type n, char_type c) : AbstractString(n, c) {}
		inline StringBase<Comparator>(char_type c) : AbstractString(1, c) {}
		inline StringBase<Comparator>(const_iterator first, const_iterator last) : AbstractString(last - first, first) {}
		inline explicit StringBase<Comparator>(MemoryPool& p) : AbstractString(p) {}
		inline StringBase<Comparator>(MemoryPool& p, const AbstractString& v) : AbstractString(p, v) {}
		inline StringBase<Comparator>(MemoryPool& p, const char_type* s, size_type l) : AbstractString(p, s, l) {}

		inline StringType& append(const StringType& str) {
			fb_assert(&str != this);
			return append(str.c_str(), str.length());
		}
		inline StringType& append(const StringType& str, size_type pos, size_type n) {
			fb_assert(&str != this);
			AdjustRange(str.length(), pos, n);
			return append(str.c_str() + pos, n);
		}
		inline StringType& append(const_pointer s, size_type n) {
			memcpy(baseAppend(n), s, n);
			return *this;
		}
		inline StringType& append(const_pointer s) {
			return append(s, strlen(s));
		}
		inline StringType& append(size_type n, char_type c) {
			memset(baseAppend(n), c, n);
			return *this;
		}
		inline StringType& append(const_iterator first, const_iterator last) {
			return append(first, last - first);
		}

		inline StringType& assign(const StringType& str) {
			fb_assert(&str != this);
			return assign(str.c_str(), str.length());
		}
		inline StringType& assign(const StringType& str, size_type pos, size_type n) {
			fb_assert(&str != this);
			AdjustRange(str.length(), pos, n);
			return assign(&str.c_str()[pos], n);
		}
		inline StringType& assign(const_pointer s, size_type n) {
			memcpy(baseAssign(n), s, n);
			return *this;
		}
		inline StringType& assign(const_pointer s) {
			return assign(s, strlen(s));
		}
		inline StringType& assign(size_type n, char_type c) {
			memset(baseAssign(n), c, n);
			return *this;
		}
		inline StringType& assign(const_iterator first, const_iterator last) {
			return assign(first, last - first);
		}

		inline StringType& operator=(const StringType& v) {
			fb_assert(&v != this);
			return assign(v);
		}
		inline StringType& operator=(const_pointer s) {
			return assign(s, strlen(s));
		}
		inline StringType& operator=(char_type c) {
			return assign(&c, 1);
		}
		inline StringType& operator+=(const StringType& v) {
			fb_assert(&v != this);
			return append(v);
		}
		inline StringType& operator+=(const_pointer s) {
			return append(s);
		}
		inline StringType& operator+=(char_type c) {
			return append(1, c);
		}
		inline StringType operator+(const StringType& v) const {
			fb_assert(&v != this);
			return add(v.c_str(), v.length());
		}
		inline StringType operator+(const_pointer s) const {
			return add(s, strlen(s));
		}
		inline StringType operator+(char_type c) const {
			return add(&c, 1);
		}

		inline StringBase<StringComparator> ToString() const {
			return StringBase<StringComparator>(c_str());
		}
		inline StringBase<PathNameComparator> ToPathName() const {
			return StringBase<PathNameComparator>(c_str());
		}

		inline StringType& insert(size_type p0, const StringType& str) {
			fb_assert(&str != this);
			return insert(p0, str.c_str(), str.length());
		}
		inline StringType& insert(size_type p0, const StringType& str, size_type pos, size_type n) {
			fb_assert(&str != this);
			AdjustRange(str.length(), pos, n);
			return insert(p0, &str.c_str()[pos], n);
		}
		inline StringType& insert(size_type p0, const_pointer s, size_type n) {
			if (p0 >= length()) {
				return append(s, n);
			}
			memcpy(baseInsert(p0, n), s, n);
			return *this;
		}
		inline StringType& insert(size_type p0, const_pointer s) {
			return insert(p0, s, strlen(s));
		}
		inline StringType& insert(size_type p0, size_type n, char_type c) {
			if (p0 >= length()) {
				return append(n, c);
			}
			memset(baseInsert(p0, n), c, n);
			return *this;
		}
		// iterator insert(iterator it, char_type c);	// what to return here?
		inline void insert(iterator it, size_type n, char_type c) {
			insert(it - c_str(), n, c);
		}
		inline void insert(iterator it, const_iterator first, const_iterator last) {
			insert(it - c_str(), first, last - first);
		}

		inline StringType& erase(size_type p0 = 0, size_type n = npos) {
			baseErase(p0, n);
			return *this;
		}
		inline iterator erase(iterator it) {
			erase(it - c_str(), 1);
			return it;
		}
		inline iterator erase(iterator first, iterator last) {
			erase(first - c_str(), last - first);
			return first;
		}

		inline StringType& replace(size_type p0, size_type n0, const StringType& str) {
			fb_assert(&str != this);
			return replace(p0, n0, str.c_str(), str.length());
		}
		inline StringType& replace(size_type p0, size_type n0, const StringType& str, size_type pos, size_type n) {
			fb_assert(&str != this);
			AdjustRange(str.length(), pos, n);
			return replace(p0, n0, &str.c_str()[pos], n);
		}
		inline StringType& replace(size_type p0, size_type n0, const_pointer s, size_type n) {
			erase(p0, n0);
			return insert(p0, s, n);
		}
		inline StringType& replace(size_type p0, size_type n0, const_pointer s) {
			return replace(p0, n0, s, strlen(s));
		}
		inline StringType& replace(size_type p0, size_type n0, size_type n, char_type c) {
			erase(p0, n0);
			return insert(p0, n, c);
		}
		inline StringType& replace(iterator first0, iterator last0, const StringType& str) {
			fb_assert(&str != this);
			return replace(first0 - c_str(), last0 - first0, str);
		}
		inline StringType& replace(iterator first0, iterator last0, const_pointer s, size_type n) {
			return replace(first0 - c_str(), last0 - first0, s, n);
		}
		inline StringType& replace(iterator first0, iterator last0, const_pointer s) {
			return replace(first0 - c_str(), last0 - first0, s);
		}
		inline StringType& replace(iterator first0, iterator last0, size_type n, char_type c) {
			return replace(first0 - c_str(), last0 - first0, n, c);
		}
		inline StringType& replace(iterator first0, iterator last0, const_iterator first, const_iterator last) {
			return replace(first0 - c_str(), last0 - first0, first, last - first);
		}

		inline StringType substr(size_type pos = 0, size_type n = npos) const {
			AdjustRange(length(), pos, n);
			return StringType(&c_str()[pos], n);
		}
		inline int compare(const StringType& str) const {
			return compare(str.c_str(), str.length());
		}
		inline int compare(size_type p0, size_type n0, const StringType& str) {
			return compare(p0, n0, str.c_str(), str.length());
		}
		inline int compare(size_type p0, size_type n0, const StringType& str, size_type pos, size_type n) {
			AdjustRange(str.length(), pos, n);
			return compare(p0, n0, &str.c_str()[pos], n);
		}
		inline int compare(const_pointer s) const {
			return compare(s, strlen(s));
		}
		int compare(size_type p0, size_type n0, const_pointer s, size_type n) const {
			AdjustRange(length(), p0, n0);
			const size_type ml = n0 < n ? n0 : n;
			const int rc = Comparator::compare(&c_str()[p0], s, ml);
			return rc ? rc : n0 - n;
		}
		int compare(const_pointer s, size_type n) const {
			const size_type ml = length() < n ? length() : n;
			const int rc = Comparator::compare(c_str(), s, ml);
			if (rc)
			{
				return rc;
			}
			const difference_type dl = length() - n;
			return (dl < 0) ? -1 : (dl > 0) ? 1 : 0;
		}

		inline bool operator< (const StringType& str) const {return compare(str) <  0;}
		inline bool operator<=(const StringType& str) const {return compare(str) <= 0;}
		inline bool operator==(const StringType& str) const {return compare(str) == 0;}
		inline bool operator>=(const StringType& str) const {return compare(str) >= 0;}
		inline bool operator> (const StringType& str) const {return compare(str) >  0;}
		inline bool operator!=(const StringType& str) const {return compare(str) != 0;}

		inline bool operator< (const char_type* str) const {return compare(str) <  0;}
		inline bool operator<=(const char_type* str) const {return compare(str) <= 0;}
		inline bool operator==(const char_type* str) const {return compare(str) == 0;}
		inline bool operator>=(const char_type* str) const {return compare(str) >= 0;}
		inline bool operator> (const char_type* str) const {return compare(str) >  0;}
		inline bool operator!=(const char_type* str) const {return compare(str) != 0;}
    };

	typedef StringBase<StringComparator> string;
	inline string operator+(string::const_pointer s, const string& str) {
		string rc(s);
		rc += str;
		return rc;
	}
	inline string operator+(string::char_type c, const string& str) {
		string rc(c);
		rc += str;
		return rc;
	}

	typedef StringBase<PathNameComparator> PathName;
	inline PathName operator+(PathName::const_pointer s, const PathName& str) {
		PathName rc(s);
		rc += str;
		return rc;
	}
	inline PathName operator+(PathName::char_type c, const PathName& str) {
		PathName rc(c);
		rc += str;
		return rc;
	}
}


#endif	// INCLUDE_FB_STRING_H
