#include "fwd.h"
#include "test.pb.h"
#include "message_protobuf.h"
#include "listener.h"
#include "connector.h"
#include "connection.h"
#include "task_pool.h"

#include <boost/asio/signal_set.hpp>

TaskPool* netTaskPool = NULL;
Listener* clientListener = NULL;
Connector* serverConnector = NULL;

class ClientListener : public Listener {

public:
	ClientListener(TaskPool& tp, const std::string& ip, const std::string& port) : Listener(tp, ip, port) {}

    /* INetHandler impl */
public:
    virtual void onConnect(Connection& c)
    {
        printf("ClientListener onConnect!\n");
    }
    virtual void onDisconnect(Connection& c)
    {
        printf("ClientListener onDisconnect!\n");
    }
    virtual void onMessage(Connection& c, Message& msg)
    {
        //printf("ServerConnector onMessage!\n");
		MessageHead h;
		msg_head(msg, h);
		if (0 == h.cmd) {
			Test::Ping ping;
			msg_body<Test::Ping>(msg, ping);
			printf("%s\n", ping.text().c_str());

			Message m;
			Test::Pong pong;
			pong.set_text("pongxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
				"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxpong");
			if (create_msg<Test::Pong>(1, pong, m))
				c.write(m);
		}
    }
};

class ServerConnector : public Connector {

public:
	ServerConnector(const std::string& ip, const std::string& port) : Connector(ip, port) {}
    
    /* INetHandler impl */
public:
    virtual void onConnect(Connection& c)
    {
        printf("ServerConnector onConnect!\n");
        
        Message m;
        Test::Ping ping;
        ping.set_text("pingxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
				"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxping");
        if (create_msg<Test::Ping>(0, ping, m))
            c.write(m);
    }
    virtual void onDisconnect(Connection& c)
    {
        printf("ServerConnector onDisconnect!\n");
    }
    virtual void onMessage(Connection& c, Message& msg)
    {
        //printf("ServerConnector onMessage!\n");
		MessageHead h;
		msg_head(msg, h);
		if (1 == h.cmd) {
			Test::Pong pong;
			msg_body<Test::Pong>(msg, pong);
			printf("%s\n", pong.text().c_str());

			Message m;
			Test::Ping ping;
			ping.set_text("pingxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
				"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxping");
			if (create_msg<Test::Ping>(0, ping, m))
				c.write(m);
		}
    }
};

static boost::asio::io_service io_service;
static boost::shared_ptr<boost::asio::io_service::work> work;

void signal_cb()
{
    printf("signal_cb\n");
    serverConnector->stop();
    clientListener->stop();
    netTaskPool->close();
    work.reset();
}

int main(int argc, char **argv)
{
    try {
        /* main thread */
        work.reset(new boost::asio::io_service::work(io_service));
        boost::asio::signal_set signals(io_service);
        signals.add(SIGINT);
        signals.add(SIGTERM);
#ifdef SIGQUIT
        signals.add(SIGQUIT);
#endif
        signals.async_wait(boost::bind(&signal_cb));

        /* net thread */
		netTaskPool = new TaskPool(10, TaskDispatchRound);
        netTaskPool->initialize();

        clientListener = new ClientListener(*netTaskPool, "127.0.0.1", "55010");
        clientListener->start();

        serverConnector = new ServerConnector("127.0.0.1", "55010");
        serverConnector->start(*netTaskPool);

		/* main thread loop */
        io_service.run();

		/* other threads join */
		delete netTaskPool;
		delete clientListener;
		delete serverConnector;
 
    } catch (std::exception& e) {
        printf("exception : %s\n", e.what());
    }
}
