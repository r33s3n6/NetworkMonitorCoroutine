#include "connection.hpp"
#include "proxy_handler.h"

#include <boost/thread.hpp>

#include <iostream>
using namespace std;
namespace proxy_tcp {



	/*
connection::connection(tcp::socket socket,
	shared_ptr<http_proxy_handler> handler_ptr)
	: _socket(std::move(socket)),
	_request_handler(handler_ptr),
	_whole_request(new string("")),
	host(""),
	_buffer({'\0'}),
	_ssl_context(boost::asio::ssl::context::sslv23)

{
	
}
*/

connection::connection(boost::asio::io_context& _io_context,
	shared_ptr<http_proxy_handler> handler_ptr,shared_ptr<certificate_manager> cert_mgr)
	: _socket(new tcp::socket(_io_context)),
	_request_handler(handler_ptr),
	_whole_request(new string("")),
	host(""),
	_buffer({ '\0' }),
	_ssl_context(boost::asio::ssl::context::sslv23),
	_cert_mgr(cert_mgr)
{
	_ssl_context.set_options(
		boost::asio::ssl::context::default_workarounds
		| boost::asio::ssl::context::no_sslv2);

	

	

	//_socket.reset(&_ssl_stream_ptr->lowest_layer());
	

}



void connection::start()
{
	//保证connection存在
	auto self = this->shared_from_this();
	co_spawn(_socket->get_executor(),
		[self]() {
			return self->_waitable_loop();
		}, detached);

}



void connection::stop()
{
	if (_socket->is_open()) {
		boost::system::error_code ignored_ec;
		_socket->shutdown(tcp::socket::shutdown_both, ignored_ec);
		_socket->close();
	}
	
}





//test

awaitable<void> connection::_waitable_loop()
{
	/*
	* 
	* 整体逻辑
	* 如果有上次剩余，则不读取
	* 读取
	* 检查完整性
	* 若为wait直接跳到开头
	* 若为chunked则.....
	
	
	*/

	/*
	* 
	* 只判断是否多次发包
	* 多次接收由handler控制，这样可以减少一遍判断
	
	*/

	//cout << "executed by thread " << boost::this_thread::get_id() << endl;
	
	boost::asio::socket_base::keep_alive option(true);
	_socket->set_option(option);

	try
	{
		boost::system::error_code ec;

		bool _with_appendix = false;
		integrity_status _status= integrity_status::broken;

		while (_keep_alive) { //循环读写

			integrity_status last_status = _status;

			shared_ptr<string> res(new string(""));
			

			
			if (!_with_appendix) {
				co_await _async_read(_is_tunnel_conn);
			}
			//若上次有一些剩余的尾巴，这次先不读而是先检查完整性，否则可能其中为最后一个报文而阻塞

			
			//_whole_request 都是解密完的http数据
	
			size_t split_pos = 0;
			//根据不同的协议使用不同的完整性检查函数
			if ((last_status == chunked) || (last_status == wait_chunked))
				_status = _chunked_integrity_check(_whole_request, split_pos);
			else
				_status = _http_integrity_check(_whole_request, split_pos);
			//appendix:[split_pos,_size)

			shared_ptr<string> remained_request;

			//分割报文
			if (split_pos < _whole_request->size()) {
				remained_request.reset(new string(
					_whole_request->substr(split_pos, _whole_request->size() - split_pos)));
				_whole_request->resize(split_pos);//清除末尾
				_with_appendix = true;
			}



			connection_behaviour _behaviour;
			switch (_status) {
			case integrity_status::chunked: //多次发包,只发不读
				_behaviour = co_await _request_handler->
					send_message(_whole_request, _is_tunnel_conn,
						(last_status == chunked||
							last_status == wait_chunked));//上一次是chunked/wait_chunked则需要强制使用旧连接
				break;
			case integrity_status::intact:
				if (_get_request_type(*_whole_request) == _CONNECT) {//connect method 单独处理直接返回，应该不可能连缀
					//DISPLAY IS NOT NECESSARY
					//format
					//CONNECT www.example.com:443 HTTP/1.1\r\n ......
					


	
					size_t host_end_pos = _whole_request->find(":");
					if (host_end_pos == string::npos) {
						host_end_pos = _whole_request->find(" HTTP");
						if (host_end_pos == string::npos) {
							throw std::runtime_error("cannot get host information");
						}
					}

					//now host_end_pos is properly set
					host = _whole_request->substr(8, host_end_pos - 8);
					//此处的host是没有端口的

					_is_tunnel_conn = true;
					*res = "HTTP/1.1 200 Connection Established\r\n\r\n";
					co_await _async_write(*res, false);

					shared_ptr<cert_key> temp = _cert_mgr->get_server_certificate(host);
					
					_ssl_context.use_certificate(boost::asio::buffer(temp->crt_bytes), boost::asio::ssl::context::pem);
					_ssl_context.use_private_key(boost::asio::buffer(temp->key_bytes), boost::asio::ssl::context::pem);
					//_ssl_context.use_certificate_file("F:/for_all.pem", boost::asio::ssl::context::pem);
					//_ssl_context.use_private_key_file("F:/for_all.key", boost::asio::ssl::context::pem);
					
					_ssl_stream_ptr = make_shared<ssl_stream>(move(*_socket), _ssl_context);//一直继承这个socket
					_socket = &_ssl_stream_ptr->next_layer();
					//for ssl connection, disable Nagle's algorithm to boost the performance
					_socket->set_option(tcp::no_delay(true));
					co_await _ssl_stream_ptr->async_handshake(boost::asio::ssl::stream_base::server
						, boost::asio::redirect_error(use_awaitable, ec));
					if (ec) {
						cout << ec.message() << endl;
						throw std::runtime_error("handshake failed");
					}


					_whole_request.reset(new string(""));
					continue;
				}//end if ==connect
				//若不为connect

				_behaviour = co_await _request_handler->
					send_message(_whole_request, _is_tunnel_conn,
						(last_status == chunked ||
							last_status == wait_chunked));//上一次是chunked/wait_chunked则需要强制使用旧连接
				break;

			case integrity_status::broken:
				_behaviour = respond_error;
				break;

			case integrity_status::wait_chunked:
			case integrity_status::wait:
				continue;

				break;
			default:
				throw std::runtime_error("integrity check failed");
			}



			//reset to wait incoming data
			if (_with_appendix) {
				//还剩一部分留在里面
				_whole_request = remained_request;
			}
			else {
				_whole_request.reset(new string(""));
			}


			//根据behaviour设置是否继续接收数据，是否直接出错返回
			switch (_behaviour) {
			case respond_and_close:
				if((last_status != chunked &&
					last_status != wait_chunked))//chunked body 不再次修改keep_alive 值
					_keep_alive = false;
				break;
			case respond_and_keep_alive:
				if ((last_status != chunked &&
					last_status != wait_chunked))
					_keep_alive = true;
				break;
			case respond_error:
				_keep_alive = false;

				_request_handler->handle_error(res); //很快，不需要异步进行
				cout << "connection::respond_error" << endl;
				co_await _async_write(*res, _is_tunnel_conn);
				continue;//自动就跳出循环了

			case ignore:
				_keep_alive = false;
				continue;//自动就跳出循环了


			case keep_receiving_data://send函数不应返回此值
				throw std::runtime_error("handler->send ERROR (presumably a bug)");
				break;
			}

			//能到这里一定没有出错
			if (_status == integrity_status::chunked) {
				continue;//跳过写入，继续读数据发送
			}


			//res指针可能直接变了，此处的res传的是引用
			_behaviour = co_await _request_handler->receive_message(res, _is_tunnel_conn);

			while (_behaviour == keep_receiving_data) {

				co_await _async_write(*res, _is_tunnel_conn);

				res.reset(new string(""));
				_behaviour = co_await _request_handler->receive_message(res, _is_tunnel_conn,true);//此时接收chunked body
			}


			switch (_behaviour) {
			case respond_error:
				_keep_alive = false;
				_request_handler->handle_error(res); //很快，不需要异步进行
				cout << "error" << endl;
				co_await _async_write(*res, _is_tunnel_conn);
				continue;//自动就跳出循环了

			case ignore:
				_keep_alive = false;
				continue;//自动就跳出循环了


			case respond_and_close:
				throw std::runtime_error(
						"switch(_behaviour) ERROR when handle"
						"write back is close (presumably a bug)");
				break;
			case respond_and_keep_alive:
				//没有任何问题
				break;
			case keep_receiving_data://此处不应该存在此值
				throw std::runtime_error("switch(_behaviour) ERROR when handle write back (presumably a bug)");
				break;
			}

			//还剩一个message 没写
			co_await _async_write(*res, _is_tunnel_conn);
		}

	}
	catch (const std::exception& e)
	{
		//NANO_LOG(WARNING,"%s", e.what());
		cout << boost::this_thread::get_id()<<":" << e.what() << endl;
	}

	stop();// Initiate graceful connection closure.
	co_return;
}

awaitable<void> connection::_async_read(bool with_ssl)
{

	boost::system::error_code ec;
	
	
	size_t bytes_transferred;
	if (with_ssl) {
		bytes_transferred = co_await _ssl_stream_ptr->async_read_some(
			boost::asio::buffer(_buffer),
			boost::asio::redirect_error(use_awaitable, ec));
		//_ssl_layer.decrypt_append(_whole_request, _buffer.data(), bytes_transferred);
	}
	else {
		bytes_transferred = co_await _socket->async_read_some(
			boost::asio::buffer(_buffer),
			boost::asio::redirect_error(use_awaitable, ec));
	}

	if (ec) {
		if (ec != boost::asio::error::eof) {

			cout << boost::this_thread::get_id() << ":" << ec.message() << endl;
			throw std::runtime_error("read failed");
		}
		else {
			throw std::runtime_error("connection closed by peer");//eof意味着无法写数据
		}
	}

	_whole_request->append(_buffer.data(), bytes_transferred);
	//cout << *_whole_request << endl;
	co_return;
}

awaitable<void> connection::_async_write(const string& data, bool with_ssl)
{
	boost::system::error_code ec;

	if (with_ssl) {
		co_await boost::asio::async_write(*_ssl_stream_ptr, boost::asio::buffer(data),
			boost::asio::redirect_error(use_awaitable, ec));
	}
	else {
		if (_ssl_stream_ptr)
			cout << "i am confused [BUG TRACK ID: 0x0f1f]" << endl;
		else
			co_await boost::asio::async_write(*_socket, boost::asio::buffer(data),
				boost::asio::redirect_error(use_awaitable, ec));
	}

	
	if (ec) {

		cout << boost::this_thread::get_id() << ":" << ec.message() 
			<< "\nwith_ssl:" << with_ssl<< endl;
		throw std::runtime_error("write failed");
	}



	co_return;
}








}