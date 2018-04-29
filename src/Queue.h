#pragma once

#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <iostream>

// Based on: https://juanchopanzacpp.wordpress.com/2013/02/26/concurrent-queue-c11/
template <typename T> class Queue
{
public:
	Queue() {
		queueMutex = new std::mutex();
		condVar = new std::condition_variable();
	}

	~Queue();

	T pop() {
		std::unique_lock<std::mutex> mlock(*queueMutex);
	    while (queue.empty())
	    {
	      condVar->wait(mlock);
	    }
	    auto item = queue.front();
	    queue.pop();
	    return item;
	}

	void pop(T& item) {
		std::unique_lock<std::mutex> mlock(*queueMutex);
	    while (queue.empty())
	    {
	      condVar->wait(mlock);
	    }
	    item = queue.front();
	    queue.pop();
	}

	void push(T&& item)
  	{
    	std::unique_lock<std::mutex> mlock(*queueMutex);
    	queue.push(std::move(item));
    	mlock.unlock();
    	condVar->notify_one();
  	}
	
	void push(T& item) {
		std::unique_lock<std::mutex> mlock(*queueMutex);
	    queue.push(item);
	    mlock.unlock();
	    condVar->notify_one();
	}

	void clear() {}

private:
	std::queue<T> queue;
	std::mutex* queueMutex;
	std::condition_variable* condVar;
	
};