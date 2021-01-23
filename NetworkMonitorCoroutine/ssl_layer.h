#pragma once
#include <memory>
#include <string>
using namespace std;

#include <boost/asio.hpp>

/*
* POOR PERFORMANCE WARNING
* 简单实现采用socket 连接boost ssl server 再取数据
* 
* 若性能成为瓶颈再替换即可
* 
*/

namespace proxy_server {

	using boost::asio::ip::tcp;
	using boost::asio::awaitable;
	using boost::asio::co_spawn;
	using boost::asio::detached;
	using boost::asio::use_awaitable;
	namespace this_coro = boost::asio::this_coro;

class ssl_layer
{
public:
	ssl_layer(const ssl_layer&) = delete;
	ssl_layer& operator=(const ssl_layer&) = delete;

	ssl_layer(){}

	awaitable<bool> _do_handshake(const string& host);
	//shared_ptr<string> ssl_decrypt(const char * data, size_t length);

	awaitable<void> decrypt_append(shared_ptr<string> str,const char* data, size_t length);

	awaitable<shared_ptr<string>> ssl_encrypt(const string& data);

private:


};

}

