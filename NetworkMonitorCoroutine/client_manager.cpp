#include "client_manager.h"


#include <boost/thread/thread.hpp>
#include <boost/bind/bind.hpp>
#include <boost/array.hpp>



namespace proxy_server {




client_manager::client_manager(size_t io_context_pool_size)
	:_io_context_pool(io_context_pool_size),
	_signals(_io_context_pool.get_io_context())
{
	_signals.add(SIGINT);
	_signals.add(SIGTERM);
#if defined(SIGQUIT)
	_signals.add(SIGQUIT);
#endif // defined(SIGQUIT)
	_signals.async_wait(boost::bind(&client_manager::_stop, this));

}

shared_ptr<client_unit> client_manager::get_client_unit()
{

	_get_client_mutex.lock();
	boost::asio::io_context& io = _io_context_pool.get_io_context();
	_get_client_mutex.unlock();
	_new_client_unit.reset(new client_unit(io));
	return _new_client_unit;
}




void client_manager::_stop()
{
	_io_context_pool.stop();
}

}