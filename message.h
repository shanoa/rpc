#ifndef MESSAGE_H_INCLUDED
#define MESSAGE_H_INCLUDED

#include "atomic_counter.h"
#include "types.h"

#include <cstddef>

#define MSG_HEAD_SIZE 8
#define MAX_MSG_SIZE 1 * 1024 * 1024

struct MessageHead {
	uint32 len;
	uint16 cmd;
#define FLAG_HAS_UID 1
	uint16 flags;
};

#define MAX_STACK_MSG_SIZE 30

class Message {

	struct Content {
		void* data;
		std::size_t size;
		atomic_counter refcnt;
	};

	enum Type {
		TypeNone = 0,
		TypeStackMsg,
		TypeHeapMsg
	};

public:
	Message();
	~Message();

	bool init(std::size_t sz);
	void copy(Message& msg);
	void close();
	void* data();
	std::size_t size();

private:
	union {
		struct {
			uint8 data[MAX_STACK_MSG_SIZE];
			uint8 size;
			uint8 type;
		} m_stackMsg;
		struct {
			Content* ct;
			uint8 unused[MAX_STACK_MSG_SIZE + 1 - sizeof(Content*)];
			uint8 type;
		} m_heapMsg;
		struct {
			uint8 unused[MAX_STACK_MSG_SIZE + 1];
			uint8 type;
		} m_base;
	} u;
};

#endif /* MESSAGE_H_INCLUDED */
