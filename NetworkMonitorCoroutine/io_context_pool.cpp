#include "io_context_pool.h"

#include <stdexcept>
#include <boost/thread/thread.hpp>
#include <boost/bind/bind.hpp>

namespace proxy_tcp {

io_context_pool::io_context_pool(std::size_t pool_size)
	: _next_io_context(0)
{
	if (pool_size == 0)
		throw std::runtime_error("io_context_pool size is 0");

	// Give all the io_contexts work to do so that their run() functions will not
	// exit until they are explicitly stopped.
	for (std::size_t i = 0; i < pool_size; ++i)
	{
		io_context_ptr io_context(new boost::asio::io_context(1));
		_io_contexts.push_back(io_context);
		//executor 提供了任务执行的上下文，具体表现为控制任务的3w 即 when where how

		// Obtain an executor with the @c outstanding_work.tracked property.
		// 当executor未被析构时，io_context 不会停止运行
		// if visual studio reports error here, please ignore it.

		
		_work_guard.push_back(boost::asio::require(io_context->get_executor() 
			,boost::asio::execution::outstanding_work.tracked));
			
		
	}
}

void io_context_pool::run()
{
	// Create a pool of threads to run all of the io_contexts.
	vector<shared_ptr<boost::thread> > threads;
	for (size_t i = 0; i < _io_contexts.size(); i++)
	{

		shared_ptr<boost::thread> thread(new boost::thread(
			boost::bind(&boost::asio::io_context::run, _io_contexts[i])));
		threads.push_back(thread);
	}

	// Wait for all threads in the pool to exit.
	for (std::size_t i = 0; i < threads.size(); ++i)
		threads[i]->join();
}


void io_context_pool::stop()
{
	// Explicitly stop all io_contexts.
	for (size_t i = 0; i < _io_contexts.size(); i++)
		_io_contexts[i]->stop();
}

boost::asio::io_context& io_context_pool::get_io_context()
{
	// Use a round-robin scheme to choose the next io_context to use.
	boost::asio::io_context& io_context = *_io_contexts[_next_io_context];
	++_next_io_context;
	if (_next_io_context == _io_contexts.size())
		_next_io_context = 0;
	return io_context;
}

}

