#ifndef NET_HANDLER_I_H_INCLUDED
#define NET_HANDLER_I_H_INCLUDED

#include "message.h"
#include "types.h"

#include <boost/shared_ptr.hpp>

class Connection;

enum HandlerType {
    HandlerListener = 0,
    HandlerConnector
};

/* net handler interface */
struct INetHandler {
    virtual HandlerType type() = 0;
    virtual void onConnect(Connection& c) = 0;
    virtual void onDisconnect(Connection& c) = 0;
    virtual void onMessage(Connection& c, Message& msg) = 0;
};

#endif /* NET_HANDLER_I_H_INCLUDED */
