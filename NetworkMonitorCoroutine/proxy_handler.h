#pragma once
/*
* 
* 代理中间层
* 用于处理断点和显示
* 
* 读取来自客户端的请求，再转发



*/


#include <memory>
#include <string>
using namespace std;

#include "connection.hpp"

#include "breakpoint_manager.h"
#include "display_filter.h"
#include "client_unit.h"

namespace proxy_tcp{

	using boost::asio::ip::tcp;
	using boost::asio::awaitable;
	using boost::asio::co_spawn;
	using boost::asio::detached;
	using boost::asio::use_awaitable;
	namespace this_coro = boost::asio::this_coro;

class http_proxy_handler
{
public:
	http_proxy_handler(const http_proxy_handler&) = delete;
	http_proxy_handler& operator=(const http_proxy_handler&) = delete;

	http_proxy_handler(breakpoint_manager& bp_mgr,
		display_filter& disp_fil, shared_ptr<client_unit> _client);

	//unified entrypoint
	awaitable<connection_behaviour> send_message(shared_ptr<string> msg,bool with_ssl,
		bool force_old_conn = false); //最后一个参数用于chunked data，如果旧连接丢失就直接失败

	//receive 不需要指定是否使用旧连接，因为断开直接失败
	awaitable<connection_behaviour> receive_message(shared_ptr<string>& rsp, 
		bool with_ssl, bool chunked_body = false);

	connection_behaviour handle_error(shared_ptr<string> result);


private:


	bool _process_header(shared_ptr<string> data, shared_ptr<string> result);

	breakpoint_manager& _breakpoint_manager;
	display_filter& _display_filter;
	shared_ptr<client_unit> _client;
	
	shared_ptr<string> host;


	int _update_id = -2;

};

}



//proxy_handler.cpp
