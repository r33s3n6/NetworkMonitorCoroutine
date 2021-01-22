#pragma once

#include <string>
#include <memory>
using namespace std;

//default behaviour: keep alive
#define CLIENT_UNIT_KEEP_ALIVE true

#include <boost/asio.hpp>


#include "connection_enums.h"

namespace proxy_server{

	using boost::asio::ip::tcp;
	using boost::asio::awaitable;
	

class client_unit
{
public:
	client_unit(const client_unit&) = delete;
	client_unit& operator=(const client_unit&) = delete;

	client_unit(boost::asio::io_context& io_context);
	~client_unit();

	

	awaitable<connection_behaviour> send_request(const string& host, const string& data, bool with_ssl, bool force_old_conn = false);
	awaitable<connection_behaviour> receive_response(shared_ptr<string>& result);//直接复用socket 不需要with_ssl 参数


	
	


private:
	

	boost::asio::io_context& _io_context;

	shared_ptr<tcp::socket> _socket;
	tcp::resolver _resolver;

	string _current_host;

	bool _current_with_ssl=false;

	tcp::resolver::results_type _endpoints;

	array<char, 8192> _buffer;

	integrity_status last_status = broken;

	shared_ptr<string> _remained_response;

	void _error_handler(boost::system::error_code ec);

	void _socket_close();

	//awaitable<bool> _check_connection(const string& host);
};



}


