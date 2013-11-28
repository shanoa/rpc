#ifndef MESSAGE_PROTOBUF_H_INCLUDED
#define MESSAGE_PROTOBUF_H_INCLUDED

#include "message.h"
#include "types.h"

#include <google/protobuf/stubs/common.h>
#include <google/protobuf/wire_format_lite_inl.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>

#include <cstddef>

#include <stdio.h>

#ifdef _WIN32
#include <Winsock2.h>
#endif /* _WIN32 */
#ifdef __unix__
#include <arpa/inet.h>
#endif

#define UID_SIZE 8

void msg_head(uint8* msg, MessageHead& h);
void msg_head(Message& msg, MessageHead& h);
bool create_msg(uint16 cmd, uint64 uid, Message& msg);
bool create_msg(uint16 cmd, Message& msg);

template<typename S>
bool create_msg(uint16 cmd, uint64 uid, S& s, Message& msg)
{
    std::size_t body_sz = s.ByteSize();
    std::size_t sz = MSG_HEAD_SIZE + UID_SIZE + body_sz;
    if (!msg.init(sz))
        return false;

    uint16* cur = (uint16*)msg.data();
    *((uint32*)cur) = htonl((uint32)(UID_SIZE + body_sz));
    cur += 2;
    *cur++ = htons((uint16)cmd);
    *cur++ = htons((uint16)FLAG_HAS_UID);
    *((uint64*)cur) = htonl((uint64)uid);
    cur += 4;

    google::protobuf::io::ArrayOutputStream arr(cur, body_sz);
    google::protobuf::io::CodedOutputStream output(&arr);
    s.SerializeToCodedStream(&output);
    return true;
}

template<typename S>
bool create_msg(uint16 cmd, S& s, Message& msg)
{
    std::size_t body_sz = s.ByteSize();
    std::size_t sz = MSG_HEAD_SIZE + body_sz;
    if (!msg.init(sz))
        return false;

    uint16* cur = (uint16*)msg.data();
    *((uint32*)cur) = htonl((uint32)(body_sz));
    cur += 2;
    *cur++ = htons((uint16)cmd);
    *cur++ = 0;

    google::protobuf::io::ArrayOutputStream arr(cur, body_sz);
    google::protobuf::io::CodedOutputStream output(&arr);
    s.SerializeToCodedStream(&output);
    return true;
}

template<typename S>
bool msg_body(Message& msg, uint64 uid, S& s)
{
    std::size_t sz = msg.size();
    if (MSG_HEAD_SIZE > sz) {
        fprintf(stderr, "msg less than head size!\n");
        return false;
    }

    uint16* cur = (uint16*)msg.data();
    uint32 len = ntohl(*((uint32*)cur));
    if (MSG_HEAD_SIZE + len != sz) {
        fprintf(stderr, "msg length error!\n");
        return false;
    }

    cur += 3;
    uint16 flags = ntohs(*cur++);
    if (0 == (flags & FLAG_HAS_UID)) {
        fprintf(stderr, "msg no uid!\n");
        return false;
    }

    google::protobuf::io::CodedInputStream input((const google::protobuf::uint8*)cur, len);
    s.ParseFromCodedStream(&input);
    return true;
}

template<typename S>
bool msg_body(Message& msg, S& s)
{
    std::size_t sz = msg.size();
    if (MSG_HEAD_SIZE > sz) {
        fprintf(stderr, "msg less than head size!\n");
        return false;
    }

    uint16* cur = (uint16*)msg.data();
    uint32 len = ntohl(*((uint32*)cur));
    if (MSG_HEAD_SIZE + len != sz) {
        fprintf(stderr, "msg length error!\n");
        return false;
    }

    cur+= 4;
    google::protobuf::io::CodedInputStream input((const google::protobuf::uint8*)cur, len);
    s.ParseFromCodedStream(&input);
    return true;
}

#endif /* MESSAGE_PROTOBUF_H_INCLUDED */
