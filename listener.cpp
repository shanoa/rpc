#include "listener.h"
#include "connection.h"
#include "task_pool.h"

#include <boost/bind.hpp>

#include <stdio.h>

Listener::Listener(TaskPool& tp, const std::string& ip, const std::string& port)
    : m_taskPool(tp)
    , m_ip(ip)
    , m_port(port)
    , m_acceptor(m_ioService)
{
}

Listener::~Listener()
{
    if (m_thread) {
        m_thread->join();
    }
}

void Listener::start()
{
    /* start thread */
    m_work.reset(new boost::asio::io_service::work(m_ioService));
    m_thread.reset(new boost::thread(boost::bind(&boost::asio::io_service::run, &m_ioService)));

    /* start acceptor */
    boost::asio::ip::tcp::resolver resolver(m_ioService);
	boost::asio::ip::tcp::resolver::query query(m_ip.c_str(), m_port.c_str());
	boost::asio::ip::tcp::endpoint ep = *resolver.resolve(query);
    m_acceptor.open(ep.protocol());
    m_acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
    m_acceptor.bind(ep);
    m_acceptor.listen();

    m_ioService.post(boost::bind(&Listener::startAccept, this));
}

void Listener::stop()
{
    m_ioService.post(boost::bind(&Listener::handleStop, this));
}

void Listener::startAccept()
{
    WorkerThreadPtr thr = m_taskPool.getNextThread();
    ConnectionPtr conn(new Connection(*this, thr));
    m_acceptor.async_accept(conn->socket(), 
            boost::bind(&Listener::handleAccept, this, conn, boost::asio::placeholders::error));
}

void Listener::handleAccept(ConnectionPtr conn, const boost::system::error_code& error)
{
    if (!m_acceptor.is_open()) {
        return;
    }
    if (!error) {
		printf("new connection established!\n");
        conn->getContext()->getIoService().post(boost::bind(&Connection::start, conn));
    }
    WorkerThreadPtr thr = m_taskPool.getNextThread();
    conn.reset(new Connection(*this, thr));
    m_acceptor.async_accept(conn->socket(),
            boost::bind(&Listener::handleAccept, this, conn, boost::asio::placeholders::error));
}

void Listener::handleStop()
{
    m_work.reset();
    m_acceptor.close();
	boost::mutex::scoped_lock lock(m_connsMutex);
	std::set<ConnectionPtr>::iterator it = m_conns.begin();
	for (; it != m_conns.end(); it++) {
		(*it)->stop();
	}
	m_conns.clear();
}

void Listener::addConn(ConnectionPtr conn)
{
	boost::mutex::scoped_lock lock(m_connsMutex);
	std::pair<std::set<ConnectionPtr>::iterator, bool> ib = m_conns.insert(conn);
	if (!ib.second) {
		// TODO log
	}
}

void Listener::delConn(ConnectionPtr conn)
{
	boost::mutex::scoped_lock lock(m_connsMutex);
	m_conns.erase(conn);
}