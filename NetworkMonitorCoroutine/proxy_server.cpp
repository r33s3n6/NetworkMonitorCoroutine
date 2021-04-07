#include "proxy_server.h"

#include <boost/thread/thread.hpp>
#include <boost/bind/bind.hpp>
#include <boost/array.hpp>
#include <boost/asio.hpp>

#include <iostream>
using namespace std;

#include "certificate_manager.h"
#include "client_unit.h"

namespace proxy_tcp {

using boost::asio::ip::tcp;
using boost::asio::awaitable;
using boost::asio::co_spawn;
using boost::asio::detached;
using boost::asio::use_awaitable;
namespace this_coro = boost::asio::this_coro;

using namespace std;



using namespace boost::asio::ip;



proxy_server::proxy_server(display_filter* df,const string& config_path)
	: _config(config_path),
	_io_context_pool(_config.io_context_pool_size),
	_io_context(_io_context_pool.get_io_context()),
	_signals(_io_context_pool.get_io_context()),
	_acceptor(_io_context_pool.get_io_context()),
	_new_proxy_conn(),
	_new_proxy_handler(),
	_display_filter(df),
	_breakpoint_manager(_config.req_filter,_config.rsp_filter)

{

	// Register to handle the signals that indicate when the server should exit.
	_signals.add(SIGINT);
	_signals.add(SIGTERM);
#if defined(SIGQUIT)
	_signals.add(SIGQUIT);
#endif // defined(SIGQUIT)
	_signals.async_wait([this](auto, auto) { this->_stop(); });


	tcp::resolver resolver(_acceptor.get_executor());

	
	boost::system::error_code ec;

	//TODO: 0.0.0.0 may accept connection from internet
	auto endpoints = resolver.resolve(_config.allow_lan_conn ? "0.0.0.0" : "127.0.0.1", _config.port,ec);
	if (ec) {
		cout << "resolve local endpoint failed" << endl;
		error_msg = "resolve local endpoint failed";
		return;
	}

	tcp::endpoint endpoint = *endpoints.begin(); //iterator of result_type

	_acceptor.open(endpoint.protocol(), ec);//return ipv4/ipv6

	if (ec) {
		cout << "acceptor open endpoint failed" << endl;
		error_msg = "acceptor open endpoint failed";
		return;
	}

	_acceptor.set_option(tcp::acceptor::reuse_address(true), ec);

	if (ec) {
		cout << "acceptor set_option failed" << endl;
		error_msg = "acceptor set_option failed";
		return;
	}

	_acceptor.bind(endpoint, ec);
	if (ec) {
		cout << "acceptor bind failed" << endl;
		error_msg = "acceptor bind failed";
		return;
	}

	_acceptor.listen(2147483647, ec);
	if (ec) {
		cout << "acceptor listen failed" << endl;
		error_msg = "acceptor listen failed";
		return;
	}



	client_unit::set_server_certificate_verify(_config.verify_server_certificate);


	//ignore the error reported by intellisense
	co_spawn(_io_context, _listener(), detached);

}




awaitable<void> proxy_server::_listener()
{
	shared_ptr<certificate_manager> cert_mgr =
		make_shared<certificate_manager>("./cert/CAcert.pem", "./cert/CAkey.pem");//TODO
	
	running = true;
	try
	{

		while (true) {


			shared_ptr<client_unit> _client(new client_unit(
				_io_context_pool.get_io_context()));
			_client->secondary_proxy = _config.secondary_proxy;
			_new_proxy_handler.reset(new http_proxy_handler(
				_breakpoint_manager, *_display_filter, _client));


			boost::system::error_code ec;

			/*
			_new_proxy_conn.reset(new connection(
				co_await _acceptor.async_accept(
					_io_context_pool.get_io_context(),
					boost::asio::redirect_error(use_awaitable, ec))
				, _new_proxy_handler));
				*/

			_new_proxy_conn.reset(new connection(
				_io_context_pool.get_io_context(), _new_proxy_handler, cert_mgr));

			co_await _acceptor.async_accept(
				_new_proxy_conn->socket(),
				boost::asio::redirect_error(use_awaitable, ec));

			if (ec) {
				cout << "Runtime Error: async_accept error!\n";
			}


			_new_proxy_conn->start();

		}

	}
	catch (const std::exception& e)
	{
		running = false;
		cout << e.what() << endl;
	}

	


}


void proxy_server::replay(shared_ptr<string> raw_req_data, bool with_bp,bool is_tunnel_conn)
{
	shared_ptr<client_unit> _client(new client_unit(
		_io_context_pool.get_io_context()));
	shared_ptr<http_proxy_handler> handler = make_shared<http_proxy_handler>(_breakpoint_manager, *_display_filter, _client);
	handler->force_breakpoint = with_bp;

	shared_ptr<certificate_manager> cert_mgr;//nullptr

	shared_ptr<connection> conn (new connection(_io_context_pool.get_io_context(), handler, cert_mgr));
	conn->set_replay_mode(raw_req_data, is_tunnel_conn);

	conn->start();
	//co_spawn(_io_context_pool.get_io_context(), _replay(raw_req_data,with_bp), detached);
}


awaitable<void> proxy_server::_replay(shared_ptr<string> raw_req_data, bool with_bp) {

	
	co_return;
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