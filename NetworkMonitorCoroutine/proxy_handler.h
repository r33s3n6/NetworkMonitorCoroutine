#pragma once



#include <memory>
#include <string>
using namespace std;

#include "connection.hpp"

#include "breakpoint_manager.h"
#include "display_filter.h"
#include "client_unit.h"

namespace proxy_server{

	using boost::asio::ip::tcp;
	using boost::asio::awaitable;
	using boost::asio::co_spawn;
	using boost::asio::detached;
	using boost::asio::use_awaitable;
	namespace this_coro = boost::asio::this_coro;


typedef enum {
	_OPTIONS,
	_HEAD,
	_GET,
	_POST,
	_PUT,
	_DELETE,
	_TRACE,
	_CONNECT //https µ•∂¿¥¶¿Ì
} request_type;



class http_proxy_handler
{
public:
	http_proxy_handler(const http_proxy_handler&) = delete;
	http_proxy_handler& operator=(const http_proxy_handler&) = delete;

	http_proxy_handler(breakpoint_manager& bp_mgr,
		display_filter& disp_fil, shared_ptr<client_unit> _client);

	awaitable<connection_behaviour> handle_request(shared_ptr<string> data, shared_ptr<string> result);
	awaitable<connection_behaviour> handle_request_as_tunnel(shared_ptr<string> data, shared_ptr<string> result);
	awaitable<connection_behaviour> handle_handshake(shared_ptr<string> data, shared_ptr<string> result);
	connection_behaviour handle_error(shared_ptr<string> result);


	//unified entrypoint

	

private:

	awaitable<connection_behaviour> _handle_request(shared_ptr<string> data, shared_ptr<string> result, connection_behaviour _behaviour);

	request_type _get_request_type(const string& data);

	bool _process_header(shared_ptr<string> data, shared_ptr<string> result);


	breakpoint_manager& _breakpoint_manager;
	display_filter& _display_filter;
	shared_ptr<client_unit> _client;
	
	shared_ptr<string> host;







};

}



//proxy_handler.cpp
