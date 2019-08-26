#include "BlockingQueue.h"
#include "Common.h"

template <class T>
void BlockingQueue<T>::Enqueue(T data)
{
	{
		std::lock_guard<std::mutex> lk(_rootObject);
		_queue.push(data);
	}
	_cv.notify_one();
}

template <class T>
void BlockingQueue<T>::Dequeue(T &dataOut)
{
	std::unique_lock<std::mutex> lk(_rootObject);
	_cv.wait(lk, [this]
	{
		return stop ? stop : HaveElement();
	});
	if (stop) return;
	dataOut = _queue.front();
	_queue.pop();
}

template <class T>
void BlockingQueue<T>::Stop()
{
	stop = true;
	_cv.notify_one();
}

template <class T>
bool BlockingQueue<T>::HaveElement()
{
	return _queue.size() > 0;
}


template <class T>
int BlockingQueue<T>::getSize()
{
	int siz = _queue.size();
	return siz;
}

template <class T>
BlockingQueue<T>::BlockingQueue()
{
}

template <class T>
BlockingQueue<T>::~BlockingQueue()
{
}

template class BlockingQueue<Common::Message*>;
template class BlockingQueue<Common::ResponseMessageParams*>;
template class BlockingQueue<Common::ResponseMessage*>;
template class BlockingQueue<Common::stopPromptMessage*>;
template class BlockingQueue<int>;
template class BlockingQueue<Common::playMessage*>;
