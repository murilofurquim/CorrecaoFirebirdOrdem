/*
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
 *  The Original Code was created by Dmitry Yemanov
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2006 Dmitry Yemanov <dimitr@users.sf.net>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 */

#ifndef JRD_DATABASE_SNAPSHOT_H
#define JRD_DATABASE_SNAPSHOT_H

#include "../common/classes/array.h"
#include "../common/classes/init.h"

namespace Jrd {

class DatabaseSnapshot
{
	enum ValueType {VALUE_GLOBAL_ID, VALUE_INTEGER, VALUE_TIMESTAMP, VALUE_STRING};

	struct DumpField
	{
		USHORT id;
		ValueType type;
		USHORT length;
		void* data;
	};

	class DumpRecord
	{
	public:
		DumpRecord() : offset(0), sizeLimit(0)
		{}

		explicit DumpRecord(int rel_id)
		{
			reset(rel_id);
		}

		void reset(int rel_id)
		{
			offset = 0;
			sizeLimit = sizeof(buffer);
			fb_assert(rel_id > 0 && rel_id <= SLONG(ULONG(MAX_UCHAR)));
			buffer[offset++] = (UCHAR) rel_id;
		}

		void assign(USHORT length, const UCHAR* ptr)
		{
			// CVC: While length is USHORT, this assertion is redundant.
			// fb_assert(length <= MAX_FORMAT_SIZE); // commented - AP - avoid gcc warning
			offset = 0;
			sizeLimit = length;
			memcpy(buffer, ptr, length);
		}

		ULONG getLength() const
		{
			return offset;
		}

		const UCHAR* getData() const
		{
			return buffer;
		}

		void storeGlobalId(int field_id, SINT64 value)
		{
			storeField(field_id, VALUE_GLOBAL_ID, sizeof(SINT64), &value);
		}

		void storeInteger(int field_id, SINT64 value)
		{
			storeField(field_id, VALUE_INTEGER, sizeof(SINT64), &value);
		}

		void storeTimestamp(int field_id, const Firebird::TimeStamp& value)
		{
			if (!value.isEmpty())
			{
				storeField(field_id, VALUE_TIMESTAMP, sizeof(ISC_TIMESTAMP), &value.value());
			}
		}

		void storeString(int field_id, const Firebird::string& value)
		{
			if (value.length())
			{
				storeField(field_id, VALUE_STRING, value.length(), value.c_str());
			}
		}

		void storeString(int field_id, const Firebird::PathName& value)
		{
			if (value.length())
			{
				storeField(field_id, VALUE_STRING, value.length(), value.c_str());
			}
		}

		void storeString(int field_id, const Firebird::MetaName& value)
		{
			if (value.length())
			{
				storeField(field_id, VALUE_STRING, value.length(), value.c_str());
			}
		}

		int getRelationId()
		{
			fb_assert(!offset);
			return (ULONG) buffer[offset++];
		}

		bool getField(DumpField& field)
		{
			fb_assert(offset);

			if (offset < sizeLimit)
			{
				field.id = (USHORT) buffer[offset++];
				field.type = (ValueType) buffer[offset++];
				fb_assert(field.type >= VALUE_GLOBAL_ID && field.type <= VALUE_STRING);
				memcpy(&field.length, buffer + offset, sizeof(USHORT));
				offset += sizeof(USHORT);
				field.data = buffer + offset;
				offset += field.length;
				return true;
			}

			return false;
		}

	private:
		void storeField(int field_id, ValueType type, size_t length, const void* value)
		{
			const size_t delta = sizeof(UCHAR) + sizeof(UCHAR) + sizeof(USHORT) + length;

			if (offset + delta > sizeLimit)
			{
				fb_assert(false);
				return;
			}

			UCHAR* ptr = buffer + offset;
			fb_assert(field_id <= SLONG(ULONG(MAX_UCHAR)));
			*ptr++ = (UCHAR) field_id;
			*ptr++ = (UCHAR) type;
			const USHORT adjusted_length = (USHORT) length;
			memcpy(ptr, &adjusted_length, sizeof(USHORT));
			ptr += sizeof(USHORT);
			memcpy(ptr, value, length);
			offset += delta;
		}

		UCHAR buffer[MAX_FORMAT_SIZE];
		ULONG offset;
		ULONG sizeLimit;
	};

	struct RelationData
	{
		int rel_id;
		RecordBuffer* data;
	};

	class SharedMemory
	{
		static const ULONG VERSION;
		static const ULONG DEFAULT_SIZE;

		struct Header
		{
			ULONG version;
			ULONG used;
			ULONG allocated;
	#ifndef WIN_NT
			MTX_T mutex;
	#endif
		};

		struct Element
		{
			ULONG processId;
			ULONG localId;
			ULONG length;
		};

		static ULONG alignOffset(ULONG absoluteOffset);

	public:
		class DumpGuard
		{
		public:
			explicit DumpGuard(SharedMemory* ptr)
				: dump(ptr)
			{
				dump->acquire();
			}

			~DumpGuard()
			{
				dump->release();
			}

		private:
			DumpGuard(const DumpGuard&);
			DumpGuard& operator=(const DumpGuard&);

			SharedMemory* dump;
		};

		SharedMemory();
		~SharedMemory();

		void acquire();
		void release();

		UCHAR* readData(Database*, MemoryPool&, ULONG&);
		ULONG setupData(Database*);
		void writeData(ULONG, ULONG, const void*);

		void cleanup(Database*);

	private:
		// copying is prohibited
		SharedMemory(const SharedMemory&);
		SharedMemory& operator =(const SharedMemory&);

		void ensureSpace(ULONG);

		static void checkMutex(const TEXT*, int);
		static void init(void*, SH_MEM_T*, bool);

		SH_MEM_T handle;
	#ifdef WIN_NT
		MTX_T mutex;
	#endif
		Header* base;
	};

	class Writer
	{
	public:
		Writer(Database* dbb, SharedMemory* _dump)
			: dump(_dump)
		{
			fb_assert(dump);
			offset = dump->setupData(dbb);
			fb_assert(offset);
		}

		void putRecord(const DumpRecord& record)
		{
			const USHORT length = (USHORT) record.getLength();
			dump->writeData(offset, sizeof(USHORT), &length);
			dump->writeData(offset, length, record.getData());
		}

	private:
		SharedMemory* dump;
		ULONG offset;
	};

	class Reader
	{
	public:
		Reader(ULONG length, UCHAR* ptr)
			: sizeLimit(length), buffer(ptr), offset(0)
		{}

		bool getRecord(DumpRecord& record)
		{
			if (offset < sizeLimit)
			{
				USHORT length;
				memcpy(&length, buffer + offset, sizeof(USHORT));
				offset += sizeof(USHORT);
				record.assign(length, buffer + offset);
				offset += length;
				return true;
			}

			return false;
		}

	private:
		ULONG sizeLimit;
		const UCHAR* buffer;
		ULONG offset;
	};

public:
	~DatabaseSnapshot();

	RecordBuffer* getData(const jrd_rel*) const;

	static DatabaseSnapshot* create(thread_db*);
	static void cleanup(Database*);
	static int blockingAst(void*);

	static void init() // for InitMutex
	{
		dump = FB_NEW(*getDefaultMemoryPool()) SharedMemory;
	}

protected:
	DatabaseSnapshot(thread_db*, MemoryPool&);

private:
	RecordBuffer* allocBuffer(thread_db*, MemoryPool&, int);
	void clearRecord(Record*);
	void putField(thread_db*, Record*, const DumpField&, int&, bool = false);

	static void dumpData(thread_db*, bool);

	static SINT64 getGlobalId(int);

	static void putDatabase(const Database*, Writer&, int);
	static bool putAttachment(const Attachment*, Writer&, int);
	static void putTransaction(const jrd_tra*, Writer&, int);
	static void putRequest(const jrd_req*, Writer&, int);
	static void putCall(const jrd_req*, Writer&, int);
	static void putStatistics(const RuntimeStatistics*, Writer&, int, int);

	static SharedMemory* dump;
	static Firebird::InitMutex<DatabaseSnapshot> startup;

	Firebird::Array<RelationData> snapshot;
	Firebird::GenericMap<Firebird::Pair<Firebird::NonPooled<SINT64, SLONG> > > idMap;
	int idCounter;
};

} // namespace

#endif // JRD_DATABASE_SNAPSHOT_H
