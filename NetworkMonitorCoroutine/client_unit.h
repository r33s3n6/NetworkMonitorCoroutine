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

	void _error_handler(boost::system::error_code ec);

	awaitable<bool> send_request(const string& host,const string& data, shared_ptr<string> result);

	awaitable<bool> send_request_ssl(const string& host, const string& data, shared_ptr<string> result);

	


private:

	void _socket_close();

	boost::asio::io_context& _io_context;

	shared_ptr<tcp::socket> _socket;
	tcp::resolver _resolver;

	string _current_host;
	tcp::resolver::results_type _endpoints;

	array<char, 8192> _buffer;

	shared_ptr<string> _whole_response;


};



}


