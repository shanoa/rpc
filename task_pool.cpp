#include "task_pool.h"

#include <boost/bind.hpp>

#include <stdio.h>

WorkerThread::WorkerThread()
{
    m_work.reset(new boost::asio::io_service::work(m_ioService));
}

WorkerThread::~WorkerThread()
{
}

bool WorkerThread::initialize(boost::shared_ptr<boost::barrier> b)
{
    m_thread.reset(new boost::thread(boost::bind(&WorkerThread::run, this, b)));
    return true;
}

static void wait(boost::shared_ptr<boost::barrier> b)
{
    b->wait();
}

void WorkerThread::run(boost::shared_ptr<boost::barrier> b)
{
    /* thread intialize */

    /* wait */
    m_ioService.post(boost::bind(&wait, b));

    /* io service run */
    m_ioService.run();
}

void WorkerThread::close()
{
    m_work.reset();
}

void WorkerThread::join()
{
    if (m_thread)
        m_thread->join();
}

TaskPool::TaskPool(uint8 nthreads, TaskDispatchPolicy policy)
    : m_running(false)
    , m_nThreads(nthreads)
    , m_policy(policy)
    , m_lastThread(0xff)
{
    m_barrier.reset(new boost::barrier(m_nThreads + 1));
}

TaskPool::~TaskPool()
{
    std::vector<WorkerThreadPtr>::iterator itr = m_threads.begin();
    for (; itr != m_threads.end(); itr++) {
        (*itr)->join();
        printf("WorkerThread::~WorkerThread!\n");
    }
    m_threads.clear();
}

void TaskPool::initialize()
{
    if (m_running){
        fprintf(stderr, "duplicate intialize!\n");
        exit(1);
    }
    if (0 == m_nThreads) {
        fprintf(stderr, "at least one worker thread!\n");
        exit(1);
    }
    m_threads.reserve(m_nThreads);
    for (uint8 i = 0; i < m_nThreads; i++) {
        WorkerThreadPtr thread(new WorkerThread);
        if (!thread->initialize(m_barrier)) {
            fprintf(stderr, "worker thread initialize failed!\n");
            exit(1);
        }
        m_threads.push_back(thread);
    }
    m_barrier->wait();
    m_running = true;
}

void TaskPool::close()
{
    if (!m_running) {
        fprintf(stderr, "not running!\n");
        return;
    }

    m_running = true;

    std::vector<WorkerThreadPtr>::iterator itr = m_threads.begin();
    for (; itr != m_threads.end(); itr++) {
        (*itr)->close();
    }
}

WorkerThreadPtr TaskPool::getNextThread(uint64 key)
{
    if (!m_running) {
        fprintf(stderr, "not running!\n");
        return WorkerThreadPtr();
    }
    if (TaskDispatchRound == m_policy) {
        m_lastThread = (m_lastThread + 1) % m_nThreads;
        return m_threads[m_lastThread];
    } else if (TaskDispatchHash == m_policy) {
        return m_threads[key % m_nThreads];
    }
    return WorkerThreadPtr();
}
