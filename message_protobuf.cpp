#include "message_protobuf.h"

void msg_head(uint8* msg, MessageHead& h)
{
    uint16* cur = (uint16*)msg;
    h.len = ntohl(*(uint32*)cur);
    cur += 2;
    h.cmd = ntohs(*cur++);
    h.flags = ntohs(*cur);
}

void msg_head(Message& msg, MessageHead& h)
{
	uint16* cur = (uint16*)msg.data();
    h.len = ntohl(*(uint32*)cur);
    cur += 2;
    h.cmd = ntohs(*cur++);
    h.flags = ntohs(*cur);
}

bool create_msg(uint16 cmd, uint64 uid, Message& msg)
{
    std::size_t sz = MSG_HEAD_SIZE + UID_SIZE;
    if(!msg.init(sz))
        return false;

    uint16* cur = (uint16*)msg.data();
    *((uint32*)cur) = htonl((uint32)UID_SIZE);
    cur += 2;
    *cur++ = htons((uint16)cmd);
    *cur++ = htons((uint16)FLAG_HAS_UID);
    *((uint64*)cur) = htonl((uint64)uid);
    return true;
}

bool create_msg(uint16 cmd, Message& msg)
{
    std::size_t sz = MSG_HEAD_SIZE;
    if (!msg.init(sz))
        return false;

    uint16* cur = (uint16*)msg.data();
    *((uint32*)cur) = htonl((uint32)UID_SIZE);
    cur += 2;
    *cur++ = htons((uint16)cmd);
    *cur++ = 0;
    return true;
}