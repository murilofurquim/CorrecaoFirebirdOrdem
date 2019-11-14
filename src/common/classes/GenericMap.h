/*
 *	PROGRAM:	Client/Server Common Code
 *	MODULE:		StringMap.h
 *	DESCRIPTION:	Secure handling of clumplet buffers
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
 *  The Original Code was created by Nickolay Samofatov
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2004 Nickolay Samofatov <nickolay@broadviewsoftware.com>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 *
 *
 */

#ifndef STRINGMAP_H
#define STRINGMAP_H

#include "../common/classes/fb_string.h"
#include "../common/classes/fb_pair.h"
#include "../common/classes/tree.h"

namespace Firebird {

// typename is necessary for GCC builds in template below, but at the same time
// it makes MSVC6 unhappy. Let's use it conditionally.
#if defined _MSC_VER && _MSC_VER < 1300
#define FB_TYPENAME_OPT
#else
#define FB_TYPENAME_OPT typename
#endif

//
// Generic map which allows to have POD and non-POD keys and values.
// The class is memory pools friendly.
//
// Usage
//
//   POD key (integer), non-POD value (string):
//     GenericMap<Pair<Right<int, string> > >
//
//   non-POD key (string), POD value (integer):
//     GenericMap<Pair<Left<string, int> > >
//
//   non-POD key (string), non-POD value (string):
//     GenericMap<Pair<Full<string, string> > >
//
template <typename KeyValuePair, typename KeyComparator = DefaultComparator<FB_TYPENAME_OPT KeyValuePair::first_type> >
class GenericMap : public AutoStorage {
public:
	typedef typename KeyValuePair::first_type KeyType;
	typedef typename KeyValuePair::second_type ValueType;

	GenericMap() : tree(&getPool()), mCount(0) { }
	GenericMap(MemoryPool& a_pool) : AutoStorage(a_pool), tree(&getPool()), mCount(0) { }
	~GenericMap() {
		clear();
	}

	void assign(GenericMap& v)
	{
		clear();

		for (bool found = v.getFirst(); found; found = v.getNext())
		{
			KeyValuePair* current_pair = v.current();
			put(current_pair->first, current_pair->second);
		}
	}

	void takeOwnership(GenericMap& from) {
		clear();

		tree = from.tree;
		mCount = from.mCount;

		if (from.tree.getFirst()) {
			while (true) {
				bool haveMore = from.tree.fastRemove();
				if (!haveMore)
					break;
			}
		}

		from.mCount = 0;
	}

	// Clear the map
	void clear() {
		if (tree.getFirst()) {
			while (true) {
				KeyValuePair* temp = tree.current();
				bool haveMore = tree.fastRemove();
				delete temp;
				if (!haveMore)
					break;
			}
		}

		mCount = 0;
	}

	// Returns true if value existed
	bool remove(const KeyType& key) {
		if (tree.locate(key)) {
			KeyValuePair* var = tree.current();
			tree.fastRemove();
			delete var;
			mCount--;
			return true;
		}

		return false;
	}

	// Returns true if value existed previously
	bool put(const KeyType& key, const ValueType& value) {
		if (tree.locate(key)) {
			tree.current()->second = value;
			return true;
		}

		KeyValuePair* var = FB_NEW(getPool()) KeyValuePair(getPool(), key, value);
		tree.add(var);
		mCount++;
		return false;
	}

	// Returns pointer to the added empty value or null when key already exists
	ValueType* put(const KeyType& key) {
		if (tree.locate(key)) {
			return 0;
		}

		KeyValuePair* var = FB_NEW(getPool()) KeyValuePair(getPool());
		var->first = key;
		tree.add(var);
		mCount++;
		return &var->second;
	}

	// Returns true if value is found
	bool get(const KeyType& key, ValueType& value) {
		if (tree.locate(key)) {
			value = tree.current()->second;
			return true;
		}

		return false;
	}

	// Returns pointer to the found value or null otherwise
	ValueType* get(const KeyType& key) {
		if (tree.locate(key)) {
			return &tree.current()->second;
		}

		return NULL;
	}

	bool getFirst() { return tree.getFirst(); }
	
	bool getLast() { return tree.getLast(); }
	
	bool getNext() { return tree.getNext(); }
	
	bool getPrev() { return tree.getPrev(); }

	KeyValuePair* current() const { return tree.current(); }

	bool exist(const KeyType& key)
	{
		return tree.locate(key);
	}

	size_t count() const { return mCount; }

	typedef BePlusTree<KeyValuePair*, KeyType, MemoryPool, FirstObjectKey<KeyValuePair>, KeyComparator> ValuesTree;
private:
	ValuesTree tree;
	size_t mCount;
};

typedef GenericMap<Pair<Full<string, string> > > StringMap;

}

#endif
