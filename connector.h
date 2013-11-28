#ifndef CONNECTOR_H_INCLUDED
#define CONNECTOR_H_INCLUDED

#include "net_handler_i.h"
#include "common_fwd.h"
#include "types.h"

#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/error.hpp>

#include <string>

class Connector : public INetHandler {
    friend class Connection;
public:
	Connector(const std::string& ip, const std::string& port);
    ~Connector();

    /* INetHandler impl */
public:
    virtual HandlerType type() { return HandlerConnector; }

public:
    void start(TaskPool& tp);
    void stop();
    void write(Message& msg);

private:
    void startConnect();
    void handleConnect(const boost::system::error_code& error);
    void handleReconnectTimer(boost::shared_ptr<boost::asio::deadline_timer> timer);

private:
    std::string m_ip;
	std::string m_port;
    ConnectionPtr m_conn;
};

#endif /* CONNECTOR_H_INCLUDED */
