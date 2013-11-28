#include "connector.h"
#include "task_pool.h"
#include "connection.h"

Connector::Connector(const std::string& ip, const std::string& port)
    : m_ip(ip)
    , m_port(port)
{
}

Connector::~Connector()
{
}

void Connector::start(TaskPool& tp)
{
    WorkerThreadPtr thr = tp.getNextThread();
    if (!thr) {
        fprintf(stderr, "Connector::start getNextThread failed!\n");
        exit(1);
    }
    m_conn.reset(new Connection(*this, thr));
    thr->getIoService().post(boost::bind(&Connector::startConnect, this));
}

void Connector::stop()
{
    if (m_conn)
        m_conn->stop();
}

void Connector::write(Message& msg)
{
    if (m_conn)
        m_conn->write(msg);
}

void Connector::startConnect()
{
    if (Connection::Stopped == m_conn->m_state) {
        return;
    } else if (Connection::Established == m_conn->m_state) {
        fprintf(stderr, "unexpected connection state == Established!\n");
        return;
    }
    printf("Connector::startConnect %s:%s ...\n", m_ip.c_str(), m_port.c_str());
	boost::asio::ip::tcp::resolver resolver(m_conn->getContext()->getIoService());
	boost::asio::ip::tcp::resolver::query query(m_ip.c_str(), m_port.c_str());
	boost::asio::ip::tcp::endpoint ep = *resolver.resolve(query);
    m_conn->socket().async_connect(ep,
            boost::bind(&Connector::handleConnect, this, boost::asio::placeholders::error));
}

#define RECONNECT_INTERVAL_SECS 5

void Connector::handleConnect(const boost::system::error_code& error)
{
    if (!error) {
        printf("connect to %s:%s success!\n", m_ip.c_str(), m_port.c_str());
        m_conn->start();
    } else {
        /* start reconnect timer */
        boost::shared_ptr<boost::asio::deadline_timer> reconnectTimer(
                new boost::asio::deadline_timer(m_conn->getContext()->getIoService()));
        reconnectTimer->expires_from_now(boost::posix_time::seconds(RECONNECT_INTERVAL_SECS));
        reconnectTimer->async_wait(boost::bind(&Connector::handleReconnectTimer, this, reconnectTimer));
    }
}

void Connector::handleReconnectTimer(boost::shared_ptr<boost::asio::deadline_timer> timer)
{
    if (Connection::Stopped == m_conn->m_state) {
        return;
    } else if (Connection::Established == m_conn->m_state) {
        fprintf(stderr, "unexpected connection state == Established2!\n");
        return;
    }
    startConnect();
}
