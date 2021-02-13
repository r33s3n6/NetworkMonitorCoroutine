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
#include <boost/asio/ssl.hpp>
#include <boost/bind/bind.hpp>
using namespace boost::asio::ip;

#include "common_functions.h"
using namespace common;

#include "connection_enums.h"
#include "certificate_manager.h"

namespace proxy_tcp{

	using boost::asio::ip::tcp;
	using boost::asio::awaitable;
	using boost::asio::co_spawn;
	using boost::asio::detached;
	using boost::asio::use_awaitable;

	namespace this_coro = boost::asio::this_coro;

	typedef boost::asio::ssl::stream<tcp::socket> ssl_stream;

class http_proxy_handler;

class connection : public enable_shared_from_this<connection>
{
public:
	//noncopyable, please use shared_ptr
	connection(const connection&) = delete;
	connection& operator=(const connection&) = delete;

	
	//explicit connection(tcp::socket socket,
	//	shared_ptr<http_proxy_handler> handler_ptr);

	//由给定的io_context来建立连接
	explicit connection(boost::asio::io_context& _io_context,
		shared_ptr<http_proxy_handler> handler_ptr, shared_ptr<certificate_manager> cert_mgr);

	void start();
	void stop();



	~connection() {
		if (_ssl_stream_ptr)
			_socket = nullptr;//避免两次释放
		else if (_socket)
			delete _socket;
		//cout << "lost connection" << endl;
	}
	tcp::socket& socket(){ return *_socket; }

	

private:

	awaitable<void> _waitable_loop();

	awaitable<void> _async_read(bool with_ssl);
	awaitable<void> _async_write(const string& data, bool with_ssl);

	//ssl_layer _ssl_layer;

	//shared_ptr<tcp::socket> _socket;
	tcp::socket* _socket;//出于无奈
	
	bool _keep_alive = true;

	shared_ptr<http_proxy_handler> _request_handler;

	array<char, 8192> _buffer;


	shared_ptr<string> _whole_request;


	bool _is_tunnel_conn = false;

	string host;

	shared_ptr<ssl_stream> _ssl_stream_ptr;
	boost::asio::ssl::context _ssl_context;

	shared_ptr<certificate_manager> _cert_mgr;
};




}// namespace proxy_tcp