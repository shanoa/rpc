#ifndef CONNECTION_H_INCLUDED
#define CONNECTION_H_INCLUDED

#include "message.h"
#include "net_handler_i.h"
#include "common_fwd.h"
#include "types.h"

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>

#include <deque>

class Connection : public boost::enable_shared_from_this<Connection> {
    friend class Connector;
    enum State { None, Established, Stopped };
public:
    Connection(INetHandler& handler, WorkerThreadPtr ctx);
    ~Connection();

    boost::asio::ip::tcp::socket& socket() { return m_socket; }
    WorkerThreadPtr getContext() { return m_ctx; }

public:
    void start();
    void stop();
    void write(Message& msg);

    /* read */
    typedef bool (Connection::*StepFunc)();
private:
    void handleRead(const boost::system::error_code& error, std::size_t bytes_transferred);
    void readBuffer();
    void getReadableBuffer(uint8*& buf, std::size_t& sz);
    std::size_t processReadedBuffer(uint8* buf, std::size_t sz);
    void nextReadStep(void* read_pos, std::size_t to_read, StepFunc step);
    bool messageHeadReady();
    bool messageBodyReady();

    /* write */
private:
    void startWrite();
    void writeBuffer();
    void handleWrite(const boost::system::error_code& error, std::size_t bytes_transferred);
    void getWritableBuffer(uint8** buf, std::size_t* sz);
    bool popWriteMessageQueue();

    /* disconnect */
private:
    void disconnect(State state = None);

private:
    State m_state;
    WorkerThreadPtr m_ctx;
    boost::asio::ip::tcp::socket m_socket;
    INetHandler& m_handler;

    /* read */
private:
    /* where store the read data. */
    uint8* m_readPos;

    /* how much data to read before taking next step. */
    std::size_t m_toRead;
    
    /* the buffer for data to decode */
    std::size_t m_readBufSize;
    uint8* m_readBuf;

    uint8* m_currReadableBuff;
    std::size_t m_currReadableSize;

    /* the reading message */
    Message m_readingMessage;
    uint8 m_readTempBuf[MSG_HEAD_SIZE];

    /* next read step */
    StepFunc m_readStep;

    /* write */
private:
	/* write cache */
    std::deque<Message> m_writeMessageQueue;
    boost::mutex m_writeMutex;
    bool m_writing;

    /* where to get the data to write from */
    uint8* m_writePos;

    /* how much to write before next step should be executed. */
    std::size_t m_toWrite;

    /* the buffer for encoded data. */
    std::size_t m_writeBufSize;
    uint8* m_writeBuf;

    uint8* m_currWritableBuff;
    std::size_t m_currWritableSize;

    /* the writing message */
    Message m_writingMessage;
};

#endif /* CONNECTION_H_INCLUDED */
