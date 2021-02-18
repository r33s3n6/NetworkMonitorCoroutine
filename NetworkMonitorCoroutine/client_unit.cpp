#include "client_unit.h"
#include "error_response.h"
#include "common_functions.h"
using namespace common;

#include <iostream>
using namespace std;


#include <boost/asio/ssl/context.hpp>
#include <wincrypt.h>
#include <tchar.h>



X509_STORE* add_windows_root_certs()
{
	HCERTSTORE hStore = CertOpenSystemStore(0, _T("ROOT"));
	if (hStore == NULL) {
		return nullptr;
	}

	X509_STORE* store = X509_STORE_new();
	PCCERT_CONTEXT pContext = NULL;
	while ((pContext = CertEnumCertificatesInStore(hStore, pContext)) != NULL) {
		// convert from DER to internal format
		X509* x509 = d2i_X509(NULL,
			(const unsigned char**)&pContext->pbCertEncoded,
			pContext->cbCertEncoded);
		if (x509 != NULL) {
			X509_STORE_add_cert(store, x509);
			X509_free(x509);
		}
	}

	CertFreeCertificateContext(pContext);
	CertCloseStore(hStore, 0);
	return store;
	
}

namespace proxy_tcp {

	using boost::asio::co_spawn;
	using boost::asio::detached;
	using boost::asio::redirect_error;
	using boost::asio::use_awaitable;

	X509_STORE* client_unit::store = nullptr;
	bool client_unit::server_certificate_verify = true;
	//X509_STORE* client_unit::store = nullptr;

void client_unit::set_server_certificate_verify(bool verify)
{
	if (verify) {
		client_unit::store = add_windows_root_certs();
	}
	else {
		client_unit::server_certificate_verify = false;
	}

}


client_unit::client_unit(boost::asio::io_context& io_context)
	:_io_context(io_context),
	_resolver(io_context),
	_socket(),
	_current_host(""),
	_ssl_context(boost::asio::ssl::context::sslv23)

{

	if (client_unit::server_certificate_verify) {

		if (client_unit::store != nullptr) {
			// attach X509_STORE to boost ssl context
			SSL_CTX_set1_cert_store(_ssl_context.native_handle(), client_unit::store);
		}
		else {
			cout << "add_windows_root_certs failed" << endl;
		}
	}

	
		

	
	
}

client_unit::~client_unit()
{
	_socket_close();
	
	/*
	if (_ssl_stream_ptr)
		_socket = nullptr;//避免两次释放
	else if (_socket)
		delete _socket;*/
	
}


/*
void client_unit::_error_handler(boost::system::error_code ec) {
	//cout << "Error: " << ec.message() << "\n";
}
*/

//返回错误时要同时返回错误信息
//host 是可能有端口号的
awaitable<connection_behaviour> client_unit::send_request(const string& host, 
	const string& data, shared_ptr<string> error_msg, bool with_ssl,bool force_old_conn, connection_protocol protocol)
{
	if (host.size() == 0) {
		*error_msg = "host is empty";
		co_return respond_error;
	}
		
	if (!force_old_conn) {//若为新连接，重置protocol
		this->protocol = protocol;
	}

	boost::system::error_code ec;

	shared_ptr<bool> is_connected(new bool(false));

	if (!force_old_conn) {

		//try to reuse the old connection
		
		if (_current_host == host && _socket) {
			*is_connected = true;//maybe

			//读取直到超时来检测连接是否仍存在，出错说明连接中断
			boost::asio::steady_timer _deadline(_io_context);

			_deadline.expires_after(std::chrono::milliseconds(100));//等待100ms

			_deadline.async_wait(
				[this, is_connected](boost::system::error_code ec) {
					if (_socket && (*is_connected))//说明仍在等待读取
						_socket->cancel();
				});

			std::size_t bytes_transferred = co_await _socket->async_read_some(
				boost::asio::buffer(_buffer),
				boost::asio::redirect_error(use_awaitable, ec));

			if (ec != boost::asio::error::operation_aborted) {//只要不是被cancel，就肯定是出错了
				_deadline.cancel();

				*is_connected = false;
				*error_msg += ec.message();
				*error_msg += "\n";
			}


		}
	}
	else {
		(*is_connected) = true;//force_old_conn 不检查连接，直接使用
	}
	

	//clear buffer
	//_whole_response.reset(new string(""));

	//try to connect
	if (!(*is_connected)) {

		if (_ssl_stream_ptr) {//https
			_ssl_stream_ptr.reset();
			_socket = nullptr;
		}
		else if (_socket) {//http
			_socket_close();
		}


			

		if (with_ssl) {
			
			_ssl_stream_ptr = make_shared<ssl_stream>(_io_context, _ssl_context);
			if (!_ssl_stream_ptr) {
				*error_msg = "ssl_stream init failed";
				co_return respond_error;
			}
			_socket = &_ssl_stream_ptr->next_layer();
			
		}
		else {
			_socket = new tcp::socket(_io_context);
		}


		



		_current_host = host;
		string _port = "80";
		string _host = "";

		size_t host_port_pos = host.find(":");
		if (host_port_pos == string::npos) {
			if (with_ssl)
				_port = "443";
			_host = host;
		}
		else {
			_host = host.substr(0, host_port_pos);
			_port = host.substr(host_port_pos+1, host.size() - host_port_pos-1);
		}



		//解析
		_endpoints = co_await _resolver.async_resolve(_host,
			_port, redirect_error(use_awaitable, ec));

		if (ec) {

			*error_msg = "Resolve failed: ";
			*error_msg += ec.message();
			co_return respond_error;
		}




		co_await boost::asio::async_connect(*_socket, _endpoints,
			redirect_error(use_awaitable, ec));

		if (ec) {
			*error_msg = "Connect failed: ";
			*error_msg += ec.message();
			co_return respond_error;
		}


		boost::asio::socket_base::keep_alive option(true);
		_socket->set_option(option);

		if (with_ssl) {
			//for ssl connection, disable Nagle's algorithm boost the performance
			_socket->set_option(tcp::no_delay(true));

			if (client_unit::server_certificate_verify && store) {//只有成功初始化store的情况下才验证
				_ssl_stream_ptr->set_verify_mode(boost::asio::ssl::verify_peer);
				_ssl_stream_ptr->set_verify_callback(boost::asio::ssl::host_name_verification(_host));
			}

			// Set SNI Hostname (many hosts need this to handshake successfully)
			SSL_set_tlsext_host_name(_ssl_stream_ptr->native_handle(), _host.c_str());
			co_await _ssl_stream_ptr->async_handshake(boost::asio::ssl::stream_base::client
				, boost::asio::redirect_error(use_awaitable, ec));
			if (ec) {
				*error_msg = "Handshake failed: ";
				*error_msg += ec.message();
				*error_msg += "Debug info: _host:"+_host;
				co_return respond_error;
			}
		}



	}
	//connect end

	//now _socket is set up

	//send request

	if (with_ssl) {
		co_await boost::asio::async_write(*_ssl_stream_ptr, boost::asio::buffer(data),
			redirect_error(use_awaitable, ec));
	}
	else {
		co_await boost::asio::async_write(*_socket, boost::asio::buffer(data),
			redirect_error(use_awaitable, ec));
	}

	if (ec) {
		*error_msg = "async_write failed: ";
		*error_msg += ec.message();
		co_return respond_error;
	}

	co_return respond_and_keep_alive;//default
	
}

//返回错误时要同时返回错误信息
awaitable<connection_behaviour> client_unit::receive_response(shared_ptr<string>& result) //不需要检查连接性，出错直接返回即可
{
	

	boost::system::error_code ec;
	try
	{

		shared_ptr<string> _whole_response(new string(""));



		while (_socket) {
			integrity_status _status;


			
			if (_remained_response && 
				_remained_response->size() != 0) {//跳过读取，直接检查完整性
				_whole_response = _remained_response;
				_remained_response.reset();
			}
			else {
				size_t bytes_transferred;
				if (_ssl_stream_ptr) {
					bytes_transferred = co_await _ssl_stream_ptr->async_read_some(
						boost::asio::buffer(_buffer),
						boost::asio::redirect_error(use_awaitable, ec));
				}
				else {
					bytes_transferred = co_await _socket->async_read_some(
						boost::asio::buffer(_buffer),
						boost::asio::redirect_error(use_awaitable, ec));
				}
				
				if (ec) {
					if (ec != boost::asio::error::eof) {
						
						*result = "async_read_some : ";
						*result += ec.message();
						*result += "\n";
						throw std::runtime_error("client unit read failed");
					}
					else {//连接已断开
						_current_host = "";
					}
				}

				_whole_response->append(_buffer.data(), bytes_transferred);

			}

			

			//now _whole_response is set up
			

			size_t split_pos = 0;
			if (protocol == websocket_handshake) {
				_status = _http_integrity_check(_whole_response, split_pos);
				protocol = websocket;
			}
			else if (protocol == websocket) {
				_status = _websocket_integrity_check(_whole_response, split_pos);
			}
			else {
				if ((last_status == chunked) || (last_status == wait_chunked))
					_status = _chunked_integrity_check(_whole_response, split_pos);
				else
					_status = _http_integrity_check(_whole_response, split_pos);
			}
			
			//appendix:[split_pos,_size)

			connection_behaviour ret = respond_and_keep_alive;

			last_status = _status;

			switch (_status) {
			case integrity_status::websocket_intact:
			case integrity_status::intact:
				ret = respond_and_keep_alive;//default
				break;
			case integrity_status::broken:
				throw std::runtime_error("response integrity check failed (broken)");
				break;
			case integrity_status::chunked:

				ret = keep_receiving_data;
				break;

			case integrity_status::wait_chunked:
			case integrity_status::wait:
				//此处的ec要不然就没错，要不然就是eof，因为其他的早已跳出
				if (ec == boost::asio::error::eof) {
					
					throw std::runtime_error("response integrity check failed(wait but eof)");
				}
				else {
					continue;
				}
				break;
			default:

				throw std::runtime_error("response integrity check failed(switch encounter default)");
			}

			if (split_pos < _whole_response->size()) {
				_remained_response.reset(new string(
					_whole_response->substr(split_pos, _whole_response->size() - split_pos)));
				_whole_response->resize(split_pos);//清除末尾
			
			}

			//type of result is shared_ptr<string>&
			result = _whole_response;

			if (!(CLIENT_UNIT_KEEP_ALIVE || 
				(ret == integrity_status::chunked)))
				_socket_close();

			co_return ret;


		}

	}
	catch (const std::exception& e)
	{

		*result += e.what();
	}
	_socket_close();
	co_return  respond_error;
	
}

void client_unit::disconnect()
{
	_socket_close();
}



void client_unit::_socket_close()
{
	boost::system::error_code ignored_ec;
	if (_socket) {
		_socket->shutdown(tcp::socket::shutdown_both, ignored_ec);
		_socket->close();
		if (_ssl_stream_ptr) {
			_socket = nullptr;//避免两次释放
			_ssl_stream_ptr.reset();
		}
		else
			delete _socket;

	}
	
	_current_host = "";
	
	
	
}







}