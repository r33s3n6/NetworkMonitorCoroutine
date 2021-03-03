#pragma once

#include <boost/asio.hpp>
#include <vector>
#include <list>

using namespace std;

namespace proxy_tcp {

/*
这其中的io_context 全部运行协程
禁止普通的异步函数直接运行在其上
*/
class io_context_pool
{
public:
	io_context_pool(const io_context_pool&) = delete;
	io_context_pool& operator=(const io_context_pool&) = delete;

	explicit io_context_pool(std::size_t pool_size);

	void run();

	void stop();

	boost::asio::io_context& get_io_context();

private:
	typedef shared_ptr<boost::asio::io_context> io_context_ptr;
	vector<io_context_ptr> _io_contexts;

	// The work-tracking executors that keep the io_contexts running.
	list<boost::asio::any_io_executor> _work_guard;

	size_t _next_io_context;
};

}//namespace proxy_tcp

