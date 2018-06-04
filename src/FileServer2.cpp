#include "FileServer2.h"

using namespace std;
using namespace net;

FileServer2::FileServer2(unsigned short port) : users()
{
    this->port = port;

    this->stateMutex = new mutex();
    this->userMutex = new mutex();
    this->cpQueue = new Queue<ReadMessage>();
    this->udpServer = new Server(port, cpQueue);
    this->shouldConsumerRun = false;
    this->consumerThread = NULL;
    this->state = fs_stopped;
    this->cleanupTimer = new Timer(true, 1000, this, 1);
}

FileServer2::~FileServer2()
{
    stop();
    stopConsumerThread();

    delete stateMutex;
    delete userMutex;
    delete udpServer;
    delete cpQueue;
    delete cleanupTimer;
}

FileServerState FileServer2::getState()
{
    unique_lock<mutex> mlock(*stateMutex);
    FileServerState s = state;
    mlock.unlock();
    return s;
}

void FileServer2::setState(FileServerState state)
{
    unique_lock<mutex> mlock(*stateMutex);
    if (this->state == state)
    {
        mlock.unlock();
        return;
    }
    Logger::debug("[FileServer2]: " + to_string(this->state) + " -> " + to_string(state));
    this->state = state;
    mlock.unlock();

    switch (state)
    {
    case fs_running:
        cleanupTimer->start();
        startConsumerThread();
        udpServer->start();
        break;

    case fs_stopped:
        cleanupTimer->stop();
        udpServer->stop();
        stopConsumerThread();
        deleteAllUsers();
        break;

    default:
        Logger::warn("Invalid FileServer2 state: " + to_string(state));
        break;
    }
}

void FileServer2::start()
{
    setState(fs_running);
}

void FileServer2::stop()
{
    setState(fs_stopped);
}

void FileServer2::onTimerTick(int identifier)
{
    if (identifier == 1)
    {
        std::unique_lock<std::mutex> mlock(*userMutex);
        auto i = users.begin();
        while (i != users.end())
        {
            if (i->second)
            {
                // If user has no clients:
                i->second->clanupClients();
                if (i->second->isEmpty())
                {
                    Logger::info("Removing user \"" + i->second->USER_NAME + "\" because he does not contain any clients.");
                    delete i->second;
                    i = users.erase(i);
                }
                else
                {
                    i++;
                }
            }
            else
            {
                i++;
            }
        }
        mlock.unlock();
    }
}

void FileServer2::startConsumerThread()
{
    shouldConsumerRun = true;
    if (consumerThread)
    {
        stopConsumerThread();
    }
    consumerThread = new thread(&FileServer2::consumerTask, this);
}

void FileServer2::stopConsumerThread()
{
    shouldConsumerRun = false;
    if (consumerThread && consumerThread->joinable() && consumerThread->get_id() != this_thread::get_id())
    {
        ReadMessage msg = ReadMessage();
        msg.msgType = 0xff;
        cpQueue->push(msg); // Push dummy message to wake up the consumer thread
        consumerThread->join();
    }

    if (consumerThread)
    {
        delete consumerThread;
    }
    consumerThread = NULL;
}

void FileServer2::consumerTask()
{
    Logger::debug("Started server consumer thread.");
    while (shouldConsumerRun)
    {
        ReadMessage msg = cpQueue->pop();
        if (!shouldConsumerRun)
        {
            return;
        }

        switch (msg.msgType)
        {
        case 1:
            onClientHelloMessage(&msg);
            break;

        default:
            Logger::warn("Server received an unknown message type: " + to_string((int)msg.msgType));
            break;
        }
    }
    Logger::debug("Stopped server consumer thread.");
}

void FileServer2::deleteAllUsers()
{
    Logger::info("Started deleteing all users...");
    std::unique_lock<std::mutex> mlock(*userMutex);
    auto i = users.begin();
    while (i != users.end())
    {
        delete i->second;
        i = users.erase(i);
    }
    mlock.unlock();
    Logger::info("Finished deleteing all users!");
}

FileServerUser *FileServer2::getUser(string userName)
{
    std::unique_lock<std::mutex> mlock(*userMutex);
    auto c = users.find(userName);
    mlock.unlock();

    if (c != users.end())
    {
        return c->second;
    }
    return NULL;
}

FileServerUser *FileServer2::addUser(string userName, string password)
{
    std::unique_lock<std::mutex> mlock(*userMutex);
    if (users[userName])
    {
        delete users[userName];
    }
    FileServerUser *user = new FileServerUser(userName, password);
    users[userName] = user;
    mlock.unlock();

    return user;
}

FileServerClient *FileServer2::findClient(unsigned int clientId)
{
    FileServerClient *client = NULL;
    std::unique_lock<std::mutex> mlock(*userMutex);
    auto i = users.begin();
    while (i != users.end())
    {
        client = i->second->getClient(clientId);
        if (client)
        {
            mlock.unlock();
            return client;
        }
        i++;
    }
    mlock.unlock();
    return NULL;
}

void FileServer2::onClientHelloMessage(ReadMessage *msg)
{
    // Check if the checksum of the received message is valid else drop it:
    if (!AbstractMessage::isChecksumValid(msg, ClientHelloMessage::CHECKSUM_OFFSET_BITS))
    {
        return;
    }

    unsigned char flags = ClientHelloMessage::getFlagsFromMessage(msg->buffer);
    if ((flags & 0b0001) != 0b0001)
    {
        Logger::info("Client hello message without \"connect requested\" received: " + to_string((int)flags));
        return;
    }

    // Get user:
    FileServerUser *user = getUser("username");
    if (!user)
    {
        user = addUser("username", "password");
    }

    unsigned int clientId = ClientHelloMessage::getClientIdFromMessage(msg->buffer);
    unsigned int portRemote = ClientHelloMessage::getPortFromMessage(msg->buffer);
    unsigned int portLocal = 2000 + clientId % 63000;
    FileServerClient *client = new FileServerClient(clientId, portLocal, portRemote, msg->senderIp, user);

    if (findClient(clientId))
    {
        client->setDeclined(0b0100);
        delete client;
        Logger::warn("Client request declined! Reason: Client ID \"" + to_string(clientId) + "\" already taken!");
        return;
    }

    client->setAccepted(0b0001);
    user->addClient(client);
    cout << "New client with id: " << client->CLIENT_ID << " accepted on port: " << client->PORT_REMOTE << endl;
}