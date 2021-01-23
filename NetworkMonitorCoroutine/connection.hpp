#pragma once

/*
* 管理连接，断开自动销毁
* 解密https流量，确保包的完整性后才会转发给handler
* 
*/


#include <memory>
#include <array>
#include <string>
#include <iostream>
using namespace std;


#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
using namespace boost::asio::ip;

#include "common_functions.h"
using namespace common;

#include "connection_enums.h"
#include "ssl_layer.h"


namespace proxy_server{

	using boost::asio::ip::tcp;
	using boost::asio::awaitable;
	using boost::asio::co_spawn;
	using boost::asio::detached;
	using boost::asio::use_awaitable;
	namespace this_coro = boost::asio::this_coro;

class http_proxy_handler;

class connection : public enable_shared_from_this<connection>
{
public:
	//noncopyable, please use shared_ptr
	connection(const connection&) = delete;
	connection& operator=(const connection&) = delete;

	
	explicit connection(tcp::socket socket,
		shared_ptr<http_proxy_handler> handler_ptr);

	//由给定的io_context来建立连接
	//connection(boost::asio::io_context& _io_context, shared_ptr<http_proxy_handler> handler_ptr);

	void start();
	void stop();



	~connection() {
		cout << "lost connection" << endl;
	}
	tcp::socket& socket(){ return _socket; }

	

private:

	awaitable<void> _waitable_loop();

	awaitable<void> _async_read(bool with_ssl);
	awaitable<void> _async_write(const string& data, bool with_ssl);

	ssl_layer _ssl_layer;

	tcp::socket _socket;
	
	bool _keep_alive = true;

	shared_ptr<http_proxy_handler> _request_handler;

	array<char, 8192> _buffer;


	shared_ptr<string> _whole_request;


	bool _is_tunnel_conn = false;

	string host;


};




}// namespace proxy_server