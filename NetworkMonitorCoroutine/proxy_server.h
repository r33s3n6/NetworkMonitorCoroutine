#pragma once

#include <ctime>

#include <boost/asio.hpp>


using namespace boost::asio::ip;

#include "connection.hpp"

#include "proxy_handler.h"
#include "io_context_pool.h"
#include "breakpoint_manager.h"


#include "display_filter.h"

#include <iostream>
#include <cstdio>
#include <string>
#include <memory>
using namespace std;

#if defined(BOOST_ASIO_ENABLE_HANDLER_TRACKING)
# define use_awaitable \
  boost::asio::use_awaitable_t(__FILE__, __LINE__, __PRETTY_FUNCTION__)
#endif






namespace proxy_tcp {



class proxy_server
{
public:
	//禁止拷贝和赋值
	proxy_server(const proxy_server&) = delete;
	proxy_server& operator=(const proxy_server&) = delete;

	explicit proxy_server(display_filter* df, const string& config_path="config.dat");

	

	config* get_config_ptr() { return &_config; }

	bool is_running() { return running; }
	string get_error_msg() { return error_msg; }
	
	void stop() { _stop(); }
	void start();//异步运行
	void replay(shared_ptr<string> raw_req_data,bool with_bp, bool is_tunnel_conn);
	//display_filter* get_display_filter() { return &_display_filter; }

private:

	config _config;

	// Initiate an asynchronous accept operation.
	awaitable<void> _listener();
	awaitable<void> _replay(shared_ptr<string> raw_req_data, bool with_bp);

	void _stop();
	io_context_pool _io_context_pool;
	
	// The signal_set is used to register for process termination notifications.
	boost::asio::signal_set _signals;

	// Acceptor used to listen for incoming connections.
	tcp::acceptor _acceptor;

	boost::asio::io_context& _io_context;

	typedef shared_ptr<connection> proxy_conn_ptr;

	proxy_conn_ptr _new_proxy_conn;

	shared_ptr<http_proxy_handler> _new_proxy_handler;

	breakpoint_manager _breakpoint_manager;

	display_filter* _display_filter;

	bool running = false;

	string error_msg;
};

}

