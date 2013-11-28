#ifndef LISTENER_H_INCLUDED
#define LISTENER_H_INCLUDED

#include "common_fwd.h"
#include "net_handler_i.h"
#include "types.h"

#include <boost/asio/ip/tcp.hpp>
#include <boost/thread/thread.hpp>
#include <boost/shared_ptr.hpp>

#include <set>

class Listener : public INetHandler {
	friend class Connection;
public:
	Listener(TaskPool& tp, const std::string& ip, const std::string& port);
    ~Listener();

    /* INetHandler impl */
public:
    virtual HandlerType type() { return HandlerListener; }

public:
    void start();
    void stop();

    /* accept */
private:
    void startAccept();
    void handleAccept(ConnectionPtr conn, const boost::system::error_code& error);

    /* stop */
private:
    void handleStop();

	/* connection manager */
private:
	void addConn(ConnectionPtr conn);
	void delConn(ConnectionPtr conn);

private:
    TaskPool& m_taskPool;
    boost::asio::io_service m_ioService;
    boost::shared_ptr<boost::thread> m_thread;
    boost::shared_ptr<boost::asio::io_service::work> m_work;
    std::string m_ip;
	std::string m_port;
    boost::asio::ip::tcp::acceptor m_acceptor;
	std::set<ConnectionPtr> m_conns;
	boost::mutex m_connsMutex;
};

#endif /* LISTENER_H_INCLUDED */
