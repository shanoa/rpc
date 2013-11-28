#include "connection.h"
#include "connector.h"
#include "listener.h"
#include "task_pool.h"
#include "message_protobuf.h"
#include "message.h"

#include <boost/make_shared.hpp>

#define READ_BATCH_SIZE 8192
#define WRITE_BATCH_SIZE 8192

Connection::Connection(INetHandler& handler, WorkerThreadPtr ctx)
    : m_state(None)
    , m_ctx(ctx)
    , m_socket(m_ctx->getIoService())
    , m_handler(handler)
	/* read */
    , m_readPos(NULL)
    , m_toRead(0)
    , m_readBufSize(READ_BATCH_SIZE)
    , m_currReadableBuff(NULL)
    , m_currReadableSize(0)
    , m_readStep(NULL)
	/* write */
    , m_writing(false)
    , m_writePos(NULL)
    , m_toWrite(0)
    , m_writeBufSize(WRITE_BATCH_SIZE)
    , m_currWritableBuff(NULL)
    , m_currWritableSize(0)
{
	/* read */
    m_readBuf = new uint8[m_readBufSize];
    nextReadStep(m_readTempBuf, (std::size_t)MSG_HEAD_SIZE, &Connection::messageHeadReady);

	/* write */
    m_writeBuf = new uint8[m_writeBufSize];
}

Connection::~Connection()
{
    delete [] m_readBuf;
	m_readingMessage.close();
    delete [] m_writeBuf;
	m_writingMessage.close();
}

void Connection::start()
{
    if (None != m_state) {
        return;
    }

    m_state = Established;

    readBuffer();

    /* connect callback */
    m_handler.onConnect(*this);
	if (HandlerListener == m_handler.type()) {
			((Listener&)(m_handler)).addConn(shared_from_this());
		}
}

void Connection::stop()
{
    m_ctx->getIoService().post(boost::bind(&Connection::disconnect, this, Stopped));
}

void Connection::write(Message& msg)
{
    {
        boost::mutex::scoped_lock lock(m_writeMutex);
        m_writeMessageQueue.push_back(Message());
        m_writeMessageQueue.back().copy(msg);
    }
	msg.close();
    m_ctx->getIoService().post(boost::bind(&Connection::startWrite, this));
}

/*********************************** read ***********************************/

void Connection::handleRead(const boost::system::error_code& error, std::size_t bytes_transferred)
{
    if (Established != m_state) {
        return;
    }
    if (!error) {
        if ((std::size_t)-1 == processReadedBuffer(m_currReadableBuff, bytes_transferred)) {
            disconnect();
            return;
        }
        readBuffer();
    } else {
        disconnect();
    }
}

void Connection::readBuffer()
{
    getReadableBuffer(m_currReadableBuff, m_currReadableSize);
    m_socket.async_read_some(
            boost::asio::buffer(m_currReadableBuff, m_currReadableSize),
            boost::bind(&Connection::handleRead,
                shared_from_this(),
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));
}

void Connection::getReadableBuffer(uint8*& buf, std::size_t& sz)
{
    if (m_toRead >= m_readBufSize) {
        buf = m_readPos;
        sz = m_toRead;
        return;
    }
    buf = m_readBuf;
    sz = m_readBufSize;
}

std::size_t Connection::processReadedBuffer(uint8* buf, std::size_t sz)
{
    /* in case of zero-copy simply adjust the pointers, no copying
     * is required. Also, run the state machine in case all the data
     * were processed.
     */
    if (buf == m_readPos) {
        m_readPos += sz;
        m_toRead -= sz;

        while (!m_toRead) {
            if (!(this->*m_readStep)()) {
                if (!m_readStep)
                    return (std::size_t)-1;
                return sz;
            }
        }
        return sz;
    }

    std::size_t pos = 0;
    while (true) {

        /* Try to get more space in the message to fill in.
         * If none is avaiable, return.
         */
        while (!m_toRead) {
            if (!(this->*m_readStep)()) {
                if (!m_readStep)
                    return (std::size_t)-1;
                return pos;
            }
        }

        if (pos == sz)
            return pos;

        std::size_t to_copy = std::min(m_toRead, sz - pos);
        memcpy(m_readPos, m_readBuf + pos, to_copy);
        m_readPos += to_copy;
        pos += to_copy;
        m_toRead -= to_copy;
    }
}

void Connection::nextReadStep(void* read_pos, std::size_t to_read, StepFunc step)
{
    m_readPos = (uint8*)read_pos;
    m_toRead = to_read;
    m_readStep = step;
}

bool Connection::messageHeadReady()
{
    MessageHead h;
    msg_head(m_readTempBuf, h);

    if (MAX_MSG_SIZE < h.len) {
        goto err;
    }
	
	m_readingMessage.close();

	if (!m_readingMessage.init((std::size_t)(MSG_HEAD_SIZE + h.len))) {
        goto err;
    }
	
	memcpy(m_readingMessage.data(), m_readTempBuf, MSG_HEAD_SIZE);

	if (0 == h.len) {
		/* message callback */
		m_handler.onMessage(*this, m_readingMessage);
		m_readingMessage.close();
		nextReadStep(m_readTempBuf, (std::size_t)MSG_HEAD_SIZE, &Connection::messageHeadReady);
		return true;
	}

    nextReadStep((uint8*)m_readingMessage.data() + MSG_HEAD_SIZE, m_readingMessage.size() - MSG_HEAD_SIZE, &Connection::messageBodyReady);
    return true;

err:
    m_readStep = NULL;
    return false;
}

bool Connection::messageBodyReady()
{
    /* message callback */
    m_handler.onMessage(*this, m_readingMessage);

	m_readingMessage.close();

    nextReadStep(m_readTempBuf, (std::size_t)MSG_HEAD_SIZE, &Connection::messageHeadReady);
    return true;
}

/*********************************** write ***********************************/

void Connection::startWrite()
{
    if (Established != m_state) {
        return;
    }
    if (!m_writing)
        writeBuffer();
}

void Connection::writeBuffer()
{
    /* If write buffer is empty, try to read new data. */
    if (!m_currWritableSize) {
        m_currWritableBuff = NULL;
        getWritableBuffer(&m_currWritableBuff, &m_currWritableSize);
        if (m_currWritableSize == 0) {
            return;
        }
    }
    m_socket.async_write_some(
            boost::asio::buffer(m_currWritableBuff, m_currWritableSize),
            boost::bind(&Connection::handleWrite,
                shared_from_this(),
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));
    m_writing = true;
}

void Connection::handleWrite(const boost::system::error_code& error, std::size_t bytes_transferred)
{
    m_writing = false;

    if (Established != m_state) {
        return;
    }
    if (!error) {
        m_currWritableBuff += bytes_transferred;
        m_currWritableSize -= bytes_transferred;
        writeBuffer();
    } else {
        disconnect();
    }
}

void Connection::getWritableBuffer(uint8** buf, std::size_t* sz)
{
    uint8* buffer = *buf ? *buf : m_writeBuf;
    std::size_t buffersize = *buf ? *sz : m_writeBufSize;

    std::size_t pos = 0;
    while (pos < buffersize) {
        /* 
         * If there are no more data to return, run the state machine.
         * If there are still no data, return what we already have in the buffer.
         */
        if (!m_toWrite) {
            if (!popWriteMessageQueue())
                break;
        }

        if (!pos && !*buf && m_toWrite >= buffersize) {
            *buf = m_writePos;
            *sz = m_toWrite;
            m_writePos = NULL;
            m_toWrite = 0;
            return;
        }

        /* copy data to the buffer. If the buffer is full, return. */
        std::size_t to_copy = std::min(m_toWrite, buffersize - pos);
        memcpy(buffer + pos, m_writePos, to_copy);
        pos += to_copy;
        m_writePos += to_copy;
        m_toWrite -= to_copy;
    }
    *buf = buffer;
    *sz = pos;
}

bool Connection::popWriteMessageQueue()
{
    bool res = false;
    m_writingMessage.close();
    {
        boost::mutex::scoped_lock lock(m_writeMutex);
        if (!m_writeMessageQueue.empty()) {
            m_writingMessage = m_writeMessageQueue.front();
            m_writeMessageQueue.pop_front();
            m_toWrite = m_writingMessage.size();
            m_writePos = (uint8*)m_writingMessage.data();
            res = true;
        }
    }
    return res;
}

/*********************************** disconnect ***********************************/

void Connection::disconnect(State state/* = None*/)
{
    if (Established == m_state) {
        m_socket.close();
        /* disconnect callback */
		m_handler.onDisconnect(*this);
		if (HandlerListener == m_handler.type()) {
			((Listener&)(m_handler)).delConn(shared_from_this());
		}
    }
    m_state = state;
    /* reconnect */
    if (HandlerConnector == m_handler.type() && None == m_state) {
        ((Connector&)(m_handler)).startConnect();
    }
}