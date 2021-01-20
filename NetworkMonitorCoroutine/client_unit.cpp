#include "client_unit.h"
#include "error_response.h"
#include "common_functions.h"
using namespace common;

#include <iostream>
using namespace std;

namespace proxy_server {

	using boost::asio::co_spawn;
	using boost::asio::detached;
	using boost::asio::redirect_error;
	using boost::asio::use_awaitable;




client_unit::client_unit(boost::asio::io_context& io_context)
	:_io_context(io_context),
	_resolver(io_context),
	_socket(),
	_current_host("")
{

	
}

client_unit::~client_unit()
{
	
}


void client_unit::_error_handler(boost::system::error_code ec) {
	cout << "Error: " << ec.message() << "\n";
}


//return if it is successful
awaitable<bool> client_unit::send_request(const string& host,
	const string& data, shared_ptr<string> result)
{

	if (host.size() == 0)
		co_return false;

	
	boost::system::error_code ec;

	//try to reuse the old connection
	shared_ptr<bool> is_connected(new bool(false));
	if (_current_host == host && _socket) {
		*is_connected = true;
		//TODO:尝试性读取来检测连接是否仍存在
		boost::asio::steady_timer _deadline(_io_context);
		
		_deadline.expires_after(std::chrono::milliseconds(100));

		_deadline.async_wait(
			[this, is_connected](boost::system::error_code ec) {
			if (_socket && (*is_connected))//说明仍在等待读取
				_socket->cancel();
			});

		std::size_t bytes_transferred = co_await _socket->async_read_some(
			boost::asio::buffer(_buffer),
			boost::asio::redirect_error(use_awaitable, ec));
		if (ec!= boost::asio::error::operation_aborted) {//只要不是被cancel，就肯定是出错了
			_deadline.cancel();
			*is_connected = false;
		}

	}
	
	//clear buffer
	//_whole_response.reset(new string(""));

	//try to connect
	if (!(*is_connected)) {
		if(_socket)
			_socket_close();
		_socket.reset(new tcp::socket(_io_context));


		//_socket->set_option(tcp::socket::keep_alive(true));
		
		_current_host = host;

		//解析
		_endpoints = co_await _resolver.async_resolve(host,
				"http", redirect_error(use_awaitable, ec));

		if (ec) {
			_error_handler(ec);
			co_return false;
		}


		

		co_await boost::asio::async_connect(*_socket, _endpoints,
			redirect_error(use_awaitable, ec));

		if (ec) {
			_error_handler(ec);
			co_return false;
		}

		

	}
	
	
	//now _socket is set up
	
	//send request
	co_await boost::asio::async_write(*_socket, boost::asio::buffer(data),
		redirect_error(use_awaitable, ec));

	if (ec) {
		_error_handler(ec);
		co_return false;
	}

	

	//=====================================read========================================
	try
	{
		
		while (true) {
			std::size_t bytes_transferred = co_await _socket->async_read_some(
				boost::asio::buffer(_buffer),
				boost::asio::redirect_error(use_awaitable, ec));
			if (ec) {
				if (ec != boost::asio::error::eof) {
					_error_handler(ec);
					throw std::runtime_error("read failed");
				}
			}

			//_whole_response->append(_buffer.data(), bytes_transferred);
			result->append(_buffer.data(), bytes_transferred);
			integrity_status _status;

			_status = _http_integrity_check(result);



			bool ret = false;

			switch (_status) {
			case integrity_status::success:
				ret= true;
				break;
			case integrity_status::failed:

				ret= false;
				break;
			case integrity_status::wait:
				if (ec == boost::asio::error::eof) {
					_error_handler(ec);
					throw std::runtime_error("response integrity check failed");
				}
				else {
					continue;
				}
				break;
			default:
				_error_handler(ec);
				throw std::runtime_error("response integrity check failed");
			}

			if (!CLIENT_UNIT_KEEP_ALIVE)
				_socket_close();

			co_return true;



		}

	}
	catch (const std::exception&)
	{

		_socket_close();
		co_return false;
	}



	*result = error_response::error404;
	co_return false;
}

awaitable<bool> client_unit::send_request_ssl(const string& host,
	const string& data, shared_ptr<string> result)
{
	*result = error_response::error404;
	co_return false;
}

void client_unit::_socket_close()
{
	boost::system::error_code ignored_ec;
	_socket->shutdown(tcp::socket::shutdown_both, ignored_ec);
	_socket->close();
	_socket.reset();
	_current_host = "";
}





}