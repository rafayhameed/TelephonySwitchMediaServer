#pragma once
#include <mutex>
#include <queue> 
#include <condition_variable>

template <class T>
class BlockingQueue
{
private:
	std::mutex _rootObject;
	std::queue<T> _queue;
	std::condition_variable _cv;
	bool stop = false;
	bool HaveElement();
public:
	void Enqueue(T data);
	void Dequeue(T &dataOut);
	void Stop();
	int getSize();
	BlockingQueue();
	~BlockingQueue();
};

