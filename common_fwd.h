#ifndef COMMON_H_INCLUDED
#define COMMON_H_INCLUDED

#include <boost/shared_ptr.hpp>

class Connection;
typedef boost::shared_ptr<Connection> ConnectionPtr;

class WorkerThread;
typedef boost::shared_ptr<WorkerThread> WorkerThreadPtr;
typedef boost::weak_ptr<WorkerThread> WorkerThreadWeakPtr;

class TaskPool;
typedef boost::shared_ptr<TaskPool> TaskPoolPtr;
typedef boost::weak_ptr<TaskPool> TaskPoolWeakPtr;

class Listener;
class Connector;

#endif /* COMMON_H_INCLUDED */
