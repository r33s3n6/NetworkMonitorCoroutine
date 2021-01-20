#pragma once
#include <memory>
#include <string>
using namespace std;

#include <boost/asio.hpp>

#include "client_unit.h"
#include "io_context_pool.h"

namespace proxy_server{

class client_manager
{
public:
	//noncopyable

	client_manager(const client_manager&) = delete;
	client_manager& operator=(const client_manager&) = delete;

	explicit client_manager(size_t io_context_pool_size);

	shared_ptr<client_unit> get_client_unit();
private:
	void _stop();

	io_context_pool _io_context_pool;

	mutex _get_client_mutex;
	// The signal_set is used to register for process termination notifications.
	boost::asio::signal_set _signals;

	shared_ptr<client_unit> _new_client_unit;

	


};

}
