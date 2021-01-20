#pragma once

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



namespace proxy_server{

	using boost::asio::ip::tcp;
	using boost::asio::awaitable;
	using boost::asio::co_spawn;
	using boost::asio::detached;
	using boost::asio::use_awaitable;
	namespace this_coro = boost::asio::this_coro;


template <typename request_handler>
class connection : public enable_shared_from_this<connection<request_handler>>
{
public:
	//noncopyable, please use shared_ptr
	connection(const connection&) = delete;
	connection& operator=(const connection&) = delete;

	//由给定的io_context来建立连接
	explicit connection(tcp::socket socket,
		shared_ptr<request_handler> handler_ptr);

	void start();
	void stop();

	~connection() {
		//cout << "lost connection" << endl;
	}
	//tcp::socket& socket(){ return _socket; }

	

private:

	awaitable<void> _waitable_loop();


	tcp::socket _socket;
	
	bool _keep_alive = true;

	shared_ptr<request_handler> _request_handler;
	array<char, 8192> _buffer;
	shared_ptr<string> _whole_request;


	//integrity_status _http_integrity_check();
	integrity_status _https_integrity_check();

	bool _is_tunnel_conn = false;
	handshake_status _handshake_status = not_begin;


};





}// namespace proxy_server

// connection.cpp


#include <boost/bind.hpp>

namespace proxy_server {

	template<typename request_handler>
	connection<request_handler>::connection(tcp::socket socket,
		shared_ptr<request_handler> handler_ptr)
		: _socket(std::move(socket)),
		_request_handler(handler_ptr),
		_whole_request(new string(""))
	{
		boost::asio::socket_base::keep_alive _ka(true);
		_socket.set_option(_ka);

	}


	template<typename request_handler>
	void connection<request_handler>::start()
	{
		//保证connection存在
		auto self = this->shared_from_this();
		co_spawn(_socket.get_executor(),
			[self](){
			return self->_waitable_loop();
		}, detached);

	}

	template<typename request_handler>
	void connection<request_handler>::stop()
	{
		boost::system::error_code ignored_ec;
		_socket.shutdown(tcp::socket::shutdown_both, ignored_ec);
	}



	template<typename request_handler>
	awaitable<void> connection<request_handler>::_waitable_loop()
	{
		//read
		//handle

		//write

		try
		{
			boost::system::error_code ec;
			while (true) {
				std::size_t bytes_transferred = co_await _socket.async_read_some(
					boost::asio::buffer(_buffer),
					boost::asio::redirect_error(use_awaitable, ec));
				if (ec) {
					if (ec != boost::asio::error::eof) {
						throw std::runtime_error("read failed");
					}//TODO eof意味着无法写数据
				}
					
				_whole_request->append(_buffer.data(), bytes_transferred);

				integrity_status _status;
				if (!_is_tunnel_conn)
					_status = _http_integrity_check(_whole_request);
				else
					_status = _https_integrity_check();

				connection_behaviour _behaviour;

				shared_ptr<string> res(new string(""));

				switch (_status) {
				case integrity_status::success:
					if (!_is_tunnel_conn)
						_behaviour = co_await _request_handler->handle_request(_whole_request, res);
					else
						_behaviour = co_await _request_handler->handle_request_as_tunnel(_whole_request, res);
					break;
				case integrity_status::failed:

					_behaviour = _request_handler->handle_error(res); //很快，不需要异步进行

					break;
				case integrity_status::https_handshake:
					_behaviour = co_await _request_handler->handle_handshake(_whole_request, res);//TODO 可能要加上handshake_status
					break;
				case integrity_status::wait:
					if (ec == boost::asio::error::eof) {
						throw std::runtime_error("integrity check failed");
					}
					else {
						continue;
					}
					break;
				default:
					throw std::runtime_error("integrity check failed");
				}

				//reset to wait incoming data
				_whole_request.reset(new string(""));

				//return data
				switch (_behaviour) {
				case respond_and_close:
					_keep_alive = false;
					break;
				case respond_and_keep_alive:
					_keep_alive = true;
					break;
				case respond_as_tunnel:
					_keep_alive = true;
					_is_tunnel_conn = true;
					break;
				case ignore:
					stop();
					co_return;
				}

				co_await boost::asio::async_write(_socket, boost::asio::buffer(*res),
					boost::asio::redirect_error(use_awaitable, ec));
				if (!ec)
				{
					if (!_keep_alive)
						stop();// Initiate graceful connection closure.
				}
				else {
					throw std::runtime_error("write failed");
				}
			}

		}
		catch (const std::exception& e)
		{
			cout << e.what() << endl;
			stop();
		}


	}

	/*

	template<typename request_handler>
	integrity_status connection<request_handler>::_http_integrity_check()
	{
		size_t header_end_pos = _whole_request->find("\r\n\r\n");
		// check header integrity
		if (header_end_pos == string::npos) {//header not complete
			return integrity_status::wait;
		}
		// header integrity assured

		//determine body length
		size_t _request_length = 0;
		size_t length_pos = _whole_request->find("Content-Length:");
		size_t encoding_type_pos = _whole_request->find("Transfer-Encoding:");

		if (length_pos != string::npos) {// body exists
			for (int i = 0; i < 63; i++) {
				char _temp = (*_whole_request)[length_pos + 15 + i];
				if (_temp <= '9' &&
					_temp >= '0') {
					_request_length *= 10;
					_request_length += (_temp - '0');
				}
				else if (_temp == '\r')
					break;
			}
		}
		else if (encoding_type_pos != string::npos) {
			size_t _end = _whole_request->find("\r\n", encoding_type_pos);
			string t = _whole_request->substr(encoding_type_pos, _end - encoding_type_pos);
			if (t.find("chunked") != string::npos) {//分块传输

				string _body = _whole_request->substr(header_end_pos + 4,
					_whole_request->size() - (header_end_pos + 4));
				auto temp_vec_ptr = string_split(_body, "\r\n");
				if (temp_vec_ptr->size() < 3) {
					return integrity_status::wait;
				}
				else if (temp_vec_ptr->size() > 3) {
					return integrity_status::failed;
				}
				if ((*temp_vec_ptr)[2].size() != 0)
					return integrity_status::failed;
				size_t body_length = hex2decimal((*temp_vec_ptr)[0].c_str());
				if ((*temp_vec_ptr)[1].size() == body_length)
					return integrity_status::success;

				return integrity_status::failed;

			}

		}

		//now _request_length is properly set

		//check whole request integrity
		if ((_whole_request->size() - (header_end_pos + 4)) == _request_length) {
			//body integrity assured
			return integrity_status::success;
		}
		else {
			//request not complete

			//no body
			if (_request_length == 0) {//此时后面多出了一些字符，此处直接返回错误

				return integrity_status::failed;
			}
			else {
				//body not complete
				return integrity_status::wait;
			}

		}

		// unexpected behaviour, return failed
		return integrity_status::failed;
	}
	*/
	template<typename request_handler>
	inline integrity_status connection<request_handler>::_https_integrity_check()
	{
		return integrity_status::https_handshake;
	}


}