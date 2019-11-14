
#ifndef JRD_THREAD_PROTO_H
#define JRD_THREAD_PROTO_H
//
//
#include "../jrd/thd.h"
#include "../jrd/sch_proto.h"

#ifdef MULTI_THREAD
#ifdef SUPERSERVER
inline void THREAD_ENTER() {
	SCH_enter();
}
inline void THREAD_EXIT() {
	SCH_exit();
}
//inline bool THREAD_VALIDATE() {
//	return SCH_validate();
//}
#else // SUPERSERVER
inline void THREAD_ENTER() {
	gds__thread_enter();
}
inline void THREAD_EXIT() {
	gds__thread_exit();
}
//inline bool THREAD_VALIDATE() {
//	return true;
//}
#endif // SUPERSERVER
#else // MULTI_THREAD
inline void THREAD_ENTER() {
}
inline void THREAD_EXIT() {
}
//inline bool THREAD_VALIDATE() {
//	return true;
//}
#endif // MULTI_THREAD

inline void THREAD_SLEEP(ULONG msecs) {
	THD_sleep(msecs);
}
inline void THREAD_YIELD() {
	THD_yield();
}

#endif // JRD_THREAD_PROTO_H
