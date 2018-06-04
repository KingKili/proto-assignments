#include "net/Server.h"

using namespace std;
using namespace net;

Server::Server(unsigned short port, Queue<ReadMessage> *cpQueue)
{
	this->port = port;
	this->cpQueue = cpQueue;
	this->sockFD = -1;
	this->state = stopped;
	this->serverThread = NULL;
	this->shouldRun = false;
}

void Server::setState(State state)
{
	if (this->state == state)
	{
		return;
	}
	this->state = state;

	switch (state)
	{
	case stopped:
		break;

	case starting:
		break;

	case running:
		break;

	case stopping:
	{
		shouldRun = false;

		// ToDo: fix segfault:
		// Send dummy message:
		/* Client c = Client("127.0.0.0", port);
		Message dummyMsg = Message{};
		dummyMsg.buffer = (unsigned char *)"The wordl is flat!";
		dummyMsg.bufferLength = 18;
		c.send(&dummyMsg); */

		if (serverThread && serverThread->joinable() && serverThread->get_id() != this_thread::get_id())
		{
			// serverThread->join();
		}
		serverThread = NULL;
		close(sockFD);
		setState(stopped);
	}
	break;

	case error:
		break;
	}
}

State Server::getState()
{
	return state;
}

void Server::stop()
{
	setState(stopping);
}

void Server::start()
{
	if (state != stopped)
	{
		cout << "Unable to start server - state != " << stopped << " state is " << state << endl;
		return;
	}
	if (serverThread)
	{
		cerr << "UDP server already running! Please stop first!" << endl;
	}
	else
	{
		setState(starting);
		shouldRun = true;
		// Start a new server thread:
		serverThread = new thread(&Server::startTask, this);
	}
}

void Server::startTask()
{
	if (sockFD > 0)
	{
		cerr << "UDP server already running! Please stop first!" << endl;
	}
	else if ((sockFD = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		cerr << "UDP socket creation failed!" << endl;
		setState(error);
	}
	else
	{
		// Fill memory block with 0:
		myAddr = {};

		myAddr.sin_family = AF_INET;
		myAddr.sin_addr.s_addr = htonl(INADDR_ANY);
		myAddr.sin_port = htons(port);

		if (bind(sockFD, (struct sockaddr *)&myAddr, sizeof(myAddr)) < 0)
		{
			cerr << "UDP server binding failed!" << endl;
			setState(error);
		}
		else
		{
			// Setup read timeout:
			struct timeval tv;
			tv.tv_sec = 1;
			tv.tv_usec = 0;
			if (setsockopt(sockFD, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
			{
				cerr << "UDP server setting read timeout failed!" << endl;
				setState(error);
			}
			else
			{
				setState(running);
				contReadStart();
			}
		}
	}
}

void Server::contReadStart()
{
	while (shouldRun)
	{
		readNextMessage();
	}
}

void Server::readNextMessage()
{
	int recvlen = -1;
	unsigned char buf[BUF_SIZE];
	struct sockaddr_in remAddr;
	socklen_t addrLen = sizeof(remAddr);

	while (shouldRun && recvlen <= 0)
	{
		recvlen = recvfrom(sockFD, buf, BUF_SIZE, 0, (struct sockaddr *)&remAddr, &addrLen);
	}

	if (recvlen > 0)
	{
		// AbstractMessage::printByteArray(buf, recvlen);
		struct ReadMessage msg = {};
		msg.bufferLength = recvlen;
		msg.buffer = buf;
		msg.msgType = buf[0] >> 4;
		// Get sender IP:
		inet_ntop(AF_INET, &(remAddr.sin_addr), msg.senderIp, INET_ADDRSTRLEN);

		// Insert in consumer producer queue:
		cpQueue->push(msg);
	}
}