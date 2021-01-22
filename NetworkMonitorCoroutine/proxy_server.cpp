
#pragma warning disable 0304

#include "proxy_server.h"

#include <boost/thread/thread.hpp>
#include <boost/bind/bind.hpp>
#include <boost/array.hpp>
#include <boost/asio.hpp>



namespace proxy_server {

using boost::asio::ip::tcp;
using boost::asio::awaitable;
using boost::asio::co_spawn;
using boost::asio::detached;
using boost::asio::use_awaitable;
namespace this_coro = boost::asio::this_coro;

using namespace std;



using namespace boost::asio::ip;



proxy_server::proxy_server(const string& address, const string& port, size_t io_context_pool_size)
	: _io_context_pool(io_context_pool_size),

	_io_context(_io_context_pool.get_io_context()),
	_signals(_io_context_pool.get_io_context()),
	_acceptor(_io_context_pool.get_io_context()),
	_new_proxy_conn(),
	_new_proxy_handler(),
	_display_filter(),
	_breakpoint_manager()

{

	// Register to handle the signals that indicate when the server should exit.
	_signals.add(SIGINT);
	_signals.add(SIGTERM);
#if defined(SIGQUIT)
	_signals.add(SIGQUIT);
#endif // defined(SIGQUIT)
	_signals.async_wait([this](auto, auto) { this->_stop(); });


	tcp::resolver resolver(_acceptor.get_executor());
	tcp::endpoint endpoint =
		*resolver.resolve(address, port).begin(); //iterator of result_type

	_acceptor.open(endpoint.protocol());//return ipv4/ipv6
	_acceptor.set_option(tcp::acceptor::reuse_address(true));
	_acceptor.bind(endpoint);
	_acceptor.listen();

	//ignore the error reported by intellisense
	co_spawn(_io_context, _listener(), detached);

}





awaitable<void> proxy_server::_listener()
{





	while (true) {


		shared_ptr<client_unit> _client(new client_unit(
			_io_context_pool.get_io_context()));
		_new_proxy_handler.reset(new http_proxy_handler(
			_breakpoint_manager, _display_filter, _client));


		boost::system::error_code ec;

		
		_new_proxy_conn.reset(new connection(
			co_await _acceptor.async_accept(
				_io_context_pool.get_io_context(),
				boost::asio::redirect_error(use_awaitable, ec))
			, _new_proxy_handler));

		if (ec) {
			cout << "Runtime Error: async_accept error!\n";
		}


		_new_proxy_conn->start();

	}


}


void proxy_server::start()
{
	_io_context_pool.run();

}



void proxy_server::_stop()
{
	_io_context_pool.stop();
}

}