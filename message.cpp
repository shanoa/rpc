#include "message.h"

#include <stdexcept>
#include <new>

#include <stdlib.h>
#include <stdio.h>

class MessageException : public std::exception
{
public:
	const char* what() const throw () { return "MessageException!"; }
};

Message::Message()
{
	u.m_base.type = TypeNone;
}

Message::~Message()
{
}

bool Message::init(std::size_t sz)
{
    if (sz <= MAX_STACK_MSG_SIZE) {
        u.m_stackMsg.type = TypeStackMsg;
        u.m_stackMsg.size = (uint8)sz;
    } else {
        u.m_heapMsg.type = TypeHeapMsg;
        u.m_heapMsg.ct = (Content*)malloc(sizeof(Content) + sz);
        if (!u.m_heapMsg.ct) {
            fprintf(stderr, "message Content alloc failed!\n");
            return false;
        }
        u.m_heapMsg.ct->data = u.m_heapMsg.ct + 1;
        u.m_heapMsg.ct->size = sz;
        new (&u.m_heapMsg.ct->refcnt) atomic_counter();
    }
    return true;
}

void Message::copy(Message& msg)
{
    close();
    if (msg.u.m_base.type == TypeHeapMsg) {
        msg.u.m_heapMsg.ct->refcnt.inc();
    }
    *this = msg;
}

void Message::close()
{
    if (u.m_base.type == TypeHeapMsg) {
        if (!u.m_heapMsg.ct->refcnt.dec()) {
            /* replacement new */
            u.m_heapMsg.ct->refcnt.~atomic_counter();
            free(u.m_heapMsg.ct);
        }
    }
    u.m_base.type = TypeNone;
}

void* Message::data()
{
    if (TypeStackMsg == u.m_base.type) {
        return u.m_stackMsg.data;
    } else if (TypeHeapMsg == u.m_base.type) {
        return u.m_heapMsg.ct->data;
    }
	throw MessageException();
}

std::size_t Message::size()
{
    if (TypeStackMsg == u.m_base.type) {
        return u.m_stackMsg.size;
    } else if (TypeHeapMsg == u.m_base.type) {
        return u.m_heapMsg.ct->size;
    }
	throw MessageException();
}
