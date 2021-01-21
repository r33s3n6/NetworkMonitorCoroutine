
#include "proxy_handler.h"

#include "common_functions.h"
#include <algorithm>
#include <list>
using namespace common;

namespace proxy_server {

	
	typedef enum {
		transparency_proxy,
		default_proxy
	}proxy_type;



	http_proxy_handler::http_proxy_handler(
		breakpoint_manager& bp_mgr,
		display_filter& disp_fil,
		shared_ptr<client_unit> _client)
		: _breakpoint_manager(bp_mgr),
		_display_filter(disp_fil),
		_client(_client)
	{
	}

	awaitable<connection_behaviour> http_proxy_handler::receive_remaining_chunked(shared_ptr<string> result, bool _with_ssl)
	{
		connection_behaviour _behaviour = co_await _client->read_chunked_data(result); //TODO:breakpoint

		co_return _behaviour;
	}


	//send chunked data
	awaitable<connection_behaviour> http_proxy_handler::send_remaining_chunked(shared_ptr<string> result, bool _with_ssl) {
		
		
		
	}


	shared_ptr<string> http_proxy_handler::_ssl_encrypt(const shared_ptr<string>& data)
	{
		return shared_ptr<string>();
	}

	shared_ptr<string> http_proxy_handler::_ssl_decrypt(const shared_ptr<string>& data)
	{
		return shared_ptr<string>();
	}

	void http_proxy_handler::_ssl_encrypt(const shared_ptr<string>& data, shared_ptr<string> result)
	{

	}

	awaitable<connection_behaviour> http_proxy_handler::handle_request_http(
		shared_ptr<string> data, shared_ptr<string> result)
	{
		host.reset(new string(""));


		request_type _type = _get_request_type(*data);
		if (_type == _CONNECT) {
			*result = "HTTP/1.1 200 Connection Established\r\n\r\n";
			//_display_filter.display(data, result);
			co_return connection_behaviour::respond_as_tunnel;
		}

		//非connect
		shared_ptr<string> _modified_data(new string(""));
		bool _keep_alive = _process_header(data, _modified_data);

		if (_modified_data->size() == 0) {
			co_return handle_error(result);
		}

		//TODO:有一说一，不需要理这个keep alive,就应该直白的返回是否错误
		if (_keep_alive)
			co_return co_await _handle_request(_modified_data, result, respond_and_keep_alive);
		else
			co_return co_await _handle_request(_modified_data, result, respond_and_close);



	}

	awaitable<connection_behaviour> http_proxy_handler::handle_request_as_tunnel(shared_ptr<string> data, shared_ptr<string> result)
	{


		shared_ptr<string> _decrypted_data = _ssl_decrypt(data);//可能甚至不需要加解密，因为boost帮忙做了这一步

		shared_ptr<string> not_encrypted_result(new string(""));
		auto _behaviour = co_await _handle_request(data, not_encrypted_result, respond_as_tunnel);
		if (_behaviour == ignore)
			co_return _behaviour;
		_ssl_encrypt(not_encrypted_result, result);//加密并直接写入result

		co_return _behaviour;
	}

	

	awaitable<connection_behaviour> http_proxy_handler::handle_request(shared_ptr<string> data, shared_ptr<string> result, bool _with_ssl)
	{
		return awaitable<connection_behaviour>();
	}

	awaitable<connection_behaviour> http_proxy_handler::handle_handshake(shared_ptr<string> data, shared_ptr<string> result)
	{
		//_display_filter.display(data, result);
		//cout << "DEBUG::handshake_refused" << endl;
		co_return connection_behaviour::ignore;
	}

	connection_behaviour http_proxy_handler::handle_error(shared_ptr<string> result, bool _with_ssl)
	{
		return connection_behaviour();
	}

	connection_behaviour http_proxy_handler::handle_error(shared_ptr<string> result)//TODO
	{
		*result = "HTTP/1.1 400 Bad Request\r\n\r\n";

		return respond_and_close;
	}






	awaitable<connection_behaviour> http_proxy_handler::_handle_request(shared_ptr<string> data, shared_ptr<string> result, connection_behaviour _behaviour)
	{

		int update_id = -2;
		if (_breakpoint_manager.check(*data)) {
			//_modified_data 在此处是可能被修改的
			update_id = co_await _display_filter.display_breakpoint_req(data);
			if (update_id == -1) {//不继续执行
				co_return connection_behaviour::ignore;
			}
		}
		else {
			update_id = _display_filter.display(data);
		}
		connection_behaviour _behaviour_res;
		if (_behaviour == respond_as_tunnel) {//https
			_behaviour_res = co_await _client->send_request_ssl(*host,*data, result); //TODO: 此处可以判断连接是否出错 //TODO: async
		}
		else {//http
			_behaviour_res = co_await _client->send_request(*host ,*data, result);//阻塞
		}


		if (_breakpoint_manager.check(*result)) {
			if (-1== co_await _display_filter.display_breakpoint_rsp(data, result, update_id)) {//result 在此处是可能被修改的
				//请求被阻断
				co_return connection_behaviour::ignore;
			}

			co_return _behaviour_res;

		}
		else {
			_display_filter.update_display(update_id, result);
		}

		co_return _behaviour_res;

	}

	request_type http_proxy_handler::_get_request_type(const string& data)
	{
		switch (data[0]) {
		case 'G'://get
			return request_type::_GET;
		case 'O'://options
			return request_type::_OPTIONS;
		case 'P'://post/put
			if (data[1] == 'O')
				return request_type::_POST;
			else
				return request_type::_PUT;
		case 'H'://head
			return request_type::_HEAD;
		case 'D'://delete
			return request_type::_DELETE;
		case 'T'://trace
			return request_type::_TRACE;
		case 'C'://connect
			return request_type::_CONNECT;
		default:
			return request_type::_GET;
		}


		return request_type::_GET;
	}


	//return is keep_alive
	bool http_proxy_handler::_process_header(shared_ptr<string> data, shared_ptr<string> result) {
		bool _keep_alive = true;

		shared_ptr<string> header(new string(""));
		shared_ptr<string> body(new string(""));

		if (!split_request(data, header, body))
			return false;


		list<string> to_be_remove_header_list{//小写
			"proxy-authenticate",
			"proxy-connection",
			"connection",
			//"transfer-encoding","te","trailers"//可以原样转发，不要徒增麻烦
			"upgrade"
		};


		//find connection and host:

		shared_ptr<vector<string>> header_vec_ptr
			= string_split(*header, "\r\n");

		string _conn_value = get_header_value(header_vec_ptr, "proxy-connection");
		if(_conn_value.size()==0)
			_conn_value = get_header_value(header_vec_ptr, "connection");

		*host = get_header_value(header_vec_ptr, "host");


		//get to be removed field

		if (_conn_value.size() != 0) {
			auto temp_vec_ptr = string_split(_conn_value, ",");

			for (auto e : *temp_vec_ptr) {
				
				string t = string_trim(e);
				if (t.size() == 0)
					continue;
				if (t == "close") {
					_keep_alive = false;
				}
				else if (t == "keep-alive") {
					_keep_alive = true;
				}
				else {
					to_be_remove_header_list.emplace_back(t);
				}
			}
		}
		




		//first process the first line, remove the host
		string first_line = (*header_vec_ptr)[0];

		size_t pos1 = first_line.find("://");
		if (pos1 != string::npos) {//非透明代理
			size_t pos0 = first_line.find(" ");
			size_t pos2 = first_line.find("/", pos1 + 3);
			(*header_vec_ptr)[0] = first_line.substr(0, pos0 + 1) +
				first_line.substr(pos2, first_line.size() - pos2);
		}
		//透明代理什么都不用做



		for (auto header : *header_vec_ptr) {//清除逐跳头部
			string temp;
			temp.resize(header.size());
			transform(header.begin(), header.end(), temp.begin(), ::tolower);
			bool skip = false;
			auto list_iter = to_be_remove_header_list.begin();
			while (list_iter != to_be_remove_header_list.end()) {
				if (temp.find(*list_iter) == 0) {
					auto temp_iter = list_iter;
					list_iter++;
					to_be_remove_header_list.erase(temp_iter);
					skip = true;
					break;
				}
				else {
					list_iter++;
				}
			}
			if (!skip) {
				result->append(header);
				result->append("\r\n");
			}

		}

		//添加到下一跳的头部

		if (CLIENT_UNIT_KEEP_ALIVE)
			result->append("Connection: keep-alive\r\n");//默认keep-alive
		else
			result->append("Connection: close\r\n");


		result->append("\r\n");//header end
		//append body
		result->append(*body);
		return _keep_alive;
	}

	



	

}