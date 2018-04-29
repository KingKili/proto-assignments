#pragma once

#include <iostream>
#include <thread>
#include "net/State.h"
#include "net/Server.h"
#include "net/Client.h"
#include "Queue.h"
#include "net/ServerHelloMessage.h"
#include "net/ClientHelloMessage.h"

enum FileClientConnectionState {
	c_disconnected,
	c_clientHello,
	c_serverHello,
	c_connected,
	c_error
};

struct FileClientConnection {
	unsigned int clientId;
	unsigned short portLocal;
	unsigned short portRemote;
	char* remoteIp;
	net::Client* udpClient;
	net::Server* udpServer;
	Queue<net::ReadMessage>* cpQueue;
	FileClientConnectionState state = c_disconnected;
};

class FileServer
{
public:
	FileServer(unsigned short port);
	void start();
	void stop();

private:
	unsigned short port;
	net::Server server;
	net::State state;
	Queue<net::ReadMessage>* cpQueue;
	bool shouldConsumerRun;
	std::thread* consumerThread;
	FileClientConnection* client;
	unsigned int clientId;

	void startConsumerThread();
	void stopConsumerThread();
	void consumerTask();
	void onClientHelloMessage(net::ReadMessage& msg);
	void sendServerHelloMessage(const FileClientConnection& client);
};