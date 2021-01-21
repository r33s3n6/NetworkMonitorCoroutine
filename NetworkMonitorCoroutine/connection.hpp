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
	bool _keep_waiting_chunked = false;
	handshake_status _handshake_status = not_begin;


};





// namespace proxy_server

// connection.cpp






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
		

		try
		{
			boost::system::error_code ec;
			
			while (true) { //循环读写
				shared_ptr<string> res(new string(""));
				connection_behaviour _behaviour;

				if (_keep_waiting_chunked) { //chunked 只需继续接收而不需要发包
					_behaviour = co_await _request_handler->receive_remaining_chunked(res, _is_tunnel_conn);
					
				}
				else {
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
					size_t split_pos = 0;

					//TODO: 这里改一下，其实tunnel无脑转发就行，转发给虚假的https服务器，
					//拿到http数据再继续做同样的内容,因此handler根本无需考虑https的任何事，包也均是完整的
					if (!_is_tunnel_conn)
						_status = _http_integrity_check(_whole_request, split_pos);//appendix:[split_pos,_size)
					else
						_status = _https_integrity_check();

					shared_ptr<string> remained_request;
					switch (_status) {
					case integrity_status::with_appendix://先分割再发送

						
						remained_request.reset(new string(
							_whole_request->substr(split_pos, _whole_request->size()- split_pos)));

						_whole_request->resize(split_pos);//清除末尾
						//特意没有break
					case integrity_status::wait_chunked: //TODO:https多次发包,只发不读
						_behaviour = co_await _request_handler->
							send_remaining_chunked(_whole_request, _is_tunnel_conn);
						break;
					case integrity_status::success:
						_behaviour = co_await _request_handler->
							handle_request(_whole_request, res, _is_tunnel_conn);
						break;
					
					case integrity_status::failed:
						_behaviour = _request_handler->
							handle_error(res, _is_tunnel_conn); //很快，不需要异步进行

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
					
					if (_status != integrity_status::with_appendix) {
						_whole_request.reset(new string(""));
					}
					else {
						_whole_request = remained_request;
						//否则还剩一部分留在里面
					}

					

				}
				

				//return data
				switch (_behaviour) {
				case respond_and_close:
					_keep_alive = false;
					_keep_waiting_chunked = false;
					break;
				case respond_and_keep_alive:
					_keep_alive = true;
					_keep_waiting_chunked = false;
					break;
				case respond_as_tunnel:
					_keep_alive = true;
					_is_tunnel_conn = true;
					_keep_waiting_chunked = false;
					break;
				case respond_and_keep_reading:
					_keep_alive = true;
					_keep_waiting_chunked = true;//只读不发
				case ignore:
					_keep_alive = false;
					_keep_waiting_chunked = false;
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

	
	template<typename request_handler>
	inline integrity_status connection<request_handler>::_https_integrity_check()
	{
		return integrity_status::https_handshake;
	}


}