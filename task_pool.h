#ifndef TASK_POOL_H_INCLUDED
#define TASK_POOL_H_INCLUDED

#include "common_fwd.h"
#include "types.h"

#include <boost/asio/io_service.hpp>
#include <boost/thread/barrier.hpp>
#include <boost/thread/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>

#include <vector>

class WorkerThread {
public:
    WorkerThread();
    ~WorkerThread();

    bool initialize(boost::shared_ptr<boost::barrier> b);
    void run(boost::shared_ptr<boost::barrier> b);
    void close();
    void join();

    boost::asio::io_service& getIoService() { return m_ioService; }

private:
    boost::asio::io_service m_ioService;
    boost::shared_ptr<boost::thread> m_thread;
    boost::shared_ptr<boost::asio::io_service::work> m_work;
};

/* task dispatch policy */
enum TaskDispatchPolicy {
    TaskDispatchRound = 0,
    TaskDispatchHash
};

class TaskPool : public boost::noncopyable {
public:
    TaskPool(uint8 nthreads, TaskDispatchPolicy policy);
    ~TaskPool();

    void initialize();
    void close();
    
    WorkerThreadPtr getNextThread(uint64 key = 0);

private:
    volatile bool m_running;
    uint8 m_nThreads;
    boost::shared_ptr<boost::barrier> m_barrier;
    std::vector<WorkerThreadPtr> m_threads;
    TaskDispatchPolicy m_policy;
    uint8 m_lastThread;
};

#endif /* TASK_POOL_H_INCLUDED */
