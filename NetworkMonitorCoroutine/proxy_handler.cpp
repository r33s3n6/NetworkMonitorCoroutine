
#include "proxy_handler.h"

#include "common_functions.h"
#include <algorithm>
#include <list>
using namespace common;

namespace proxy_tcp {

	



	http_proxy_handler::http_proxy_handler(
		breakpoint_manager& bp_mgr,
		display_filter& disp_fil,
		shared_ptr<client_unit> _client)
		: _breakpoint_manager(bp_mgr),
		_display_filter(disp_fil),
		_client(_client)
	{
		_session_info.reset();
	}

	connection_behaviour http_proxy_handler::handle_error(shared_ptr<string> result,shared_ptr<string> err_data)//TODO:response,加入req信息
	{
		*result = "HTTP/1.1 400 Bad Request\r\n\r\n";

		if (!_session_info) {
			_session_info = make_shared<session_info>();
			
			_session_info->raw_req_data = err_data;
			_display_filter.display(_session_info);
			_session_info->new_data = make_shared<string>("Request Data Broken");
		}

		_session_info->raw_rsp_data = make_shared<string>(*result);
		_display_filter.update_display_error(_session_info);

		//cout << *_session_info->new_data << endl;
		return respond_and_close;
	}

	awaitable<connection_behaviour> http_proxy_handler::send_message(shared_ptr<string> msg,
		bool with_ssl, bool force_old_conn,bool request_end)
	{
		

		
		//一定不是connect method


		bool _keep_alive = true;


		shared_ptr<string> _modified_data(new string(""));

		

		if (!_session_info) {//说明是一次全新的请求
			host.reset(new string(""));
			_conn_protocol = http;
			_keep_alive = _process_header(msg, _modified_data);//在其中host,_conn_protocol被修改

			if (_modified_data->size() == 0) {
				co_return respond_error;
			}

			_session_info = make_shared<session_info>();

			_session_info->new_data = _modified_data;
			_session_info->raw_req_data = _modified_data;
			if (_conn_protocol == http) {
				if (with_ssl) {
					_session_info->protocol = "https";
				}
				else {
					_session_info->protocol = "http";
				}
			}
			else if (_conn_protocol == websocket_handshake) {
				if (with_ssl) {
					_session_info->protocol = "wss";
				}
				else {
					_session_info->protocol = "ws";
				}
			}
			

			_session_info->proxy_handler_ptr = shared_from_this();
			if (_breakpoint_manager.check(_session_info,true)) {//通过请求头来拦截 以后再说body的事吧 TODO
				//断点
				_session_info->send_behaviour = intercept;
			}
			else {//新显示 
				_session_info->send_behaviour = pass;
				
			}
			_display_filter.display(_session_info);
		}
		else {//是某次的后续，因此直接更新
			_session_info->raw_req_data->append(*msg);
			_modified_data = msg;

			_session_info->new_data = _modified_data;

			_display_filter.update_display_req(_session_info);
			if (_conn_protocol==websocket) {
				force_old_conn = true;
			}
		}

		

		if (request_end) {
			_display_filter.complete_req(_session_info);
		}

		connection_behaviour _behaviour = respond_and_keep_alive;

		shared_ptr<string> _error_msg = make_shared<string>("");

		if (_session_info->send_behaviour == intercept) {
			if (request_end) {
				size_t retry_count = 0;
				auto ex = co_await boost::asio::this_coro::executor;
				boost::asio::steady_timer timer(ex);
				while (retry_count < timeout 
					&& _session_info->send_behaviour == intercept) {
					timer.expires_after(std::chrono::milliseconds(1000));
					co_await timer.async_wait(use_awaitable);
				}
			}
		}
		

		switch (_session_info->send_behaviour) {
		case pass:
			_behaviour = co_await _client->send_request(
				*host, *_modified_data, _error_msg, with_ssl, force_old_conn, _conn_protocol);
			break;
		case pass_after_intercept:
			_behaviour = co_await _client->send_request(
				*host, *_session_info->raw_req_data, _error_msg, with_ssl, force_old_conn, _conn_protocol);
			_session_info->send_behaviour = pass;
			break;
		case drop:
			_session_info.reset();
			_client->disconnect();
			co_return connection_behaviour::ignore;
		case intercept://not end of request
			break;
			
		}
		

		if (_behaviour == respond_error) {
			shared_ptr<string> complete_error_msg = make_shared<string>("proxy_handler::send_message::_client::send_request ERROR : ");
			complete_error_msg->append(*_error_msg);
			_session_info->new_data = complete_error_msg;
			_display_filter.update_display_error(_session_info);
			co_return respond_error;
		}
		else {
			if (_conn_protocol == websocket_handshake)
				co_return protocol_websocket;
			if (_keep_alive)
				co_return respond_and_keep_alive;
			else
				co_return respond_and_close;

		}



		
	}

	awaitable<connection_behaviour> http_proxy_handler::receive_message(
		shared_ptr<string>& rsp, bool with_ssl, bool chunked_body)
	{
		//必然有_session_info
		//assert(!_session_info)

		connection_behaviour _behaviour;
		_behaviour = co_await _client->receive_response(rsp);
		if (_behaviour == respond_error) {
			shared_ptr<string> complete_error_msg = make_shared<string>("proxy_handler::receive_message::_client::receive_response ERROR : ");
			complete_error_msg->append(*rsp);
			_session_info->new_data = complete_error_msg;
			_display_filter.update_display_error(_session_info);

			_session_info.reset();
			_client->disconnect();
			co_return respond_error;
		}

		_session_info->new_data = rsp;

		if (_conn_protocol == websocket)
			chunked_body = true;

		if (!chunked_body) {//response begin
			
			
			_session_info->raw_rsp_data = rsp;
			
			if (_breakpoint_manager.check(_session_info,false)) {
				_session_info->receive_behaviour = intercept;//断点
			}
			else {
				_session_info->receive_behaviour = pass;
			}
			
			_display_filter.display_rsp(_session_info);
			if (_conn_protocol == websocket_handshake) {
				_conn_protocol = websocket;
			}

		}
		else {
			_session_info->raw_rsp_data->append(*rsp);
			_display_filter.update_display_rsp(_session_info);
		}

		

		
		if (_conn_protocol != websocket && _behaviour != keep_receiving_data) {//end of the request
			_display_filter.complete_rsp(_session_info);
		}

		if (_session_info->receive_behaviour == intercept) {
			if (_behaviour != keep_receiving_data) {
				size_t retry_count = 0;
				auto ex = co_await boost::asio::this_coro::executor;
				boost::asio::steady_timer timer(ex);
				while (retry_count < timeout
					&& _session_info->receive_behaviour == intercept) {
					timer.expires_after(std::chrono::milliseconds(1000));
					co_await timer.async_wait(use_awaitable);
				}
			}
		}


		switch (_session_info->receive_behaviour) {
		case pass:
			break;
		case pass_after_intercept:
			rsp = _session_info->raw_rsp_data;
			_session_info->receive_behaviour = pass;
			break;
		case drop:
			_session_info.reset();
			_client->disconnect();
			co_return connection_behaviour::ignore;
		case intercept://not end of response
			rsp.reset(new string(""));//TODO: connection 若rsp为空则不写入
			break;
		}

		if((_conn_protocol==http)&&(_behaviour != keep_receiving_data)){
			
			_session_info.reset();
		}

		co_return _behaviour;
	}



	//=====================common functions=============================

	


	//return is keep_alive
	bool http_proxy_handler::_process_header(shared_ptr<string> data, shared_ptr<string> result) {
		bool _keep_alive = true;
		_conn_protocol = http;
		shared_ptr<string> header(new string(""));
		shared_ptr<string> body(new string(""));

		if (!split_request(data, header, body))
			return false;


		list<string> to_be_removed_header_list{//小写
			"proxy-authenticate",
			"proxy-connection",
			"connection",
			//"upgrade",
			"accept-encoding"//TODO: 解决gzip, br, deflate后再加回去吧
			//"transfer-encoding","te","trailers"//可以原样转发，不要徒增麻烦
		};


		//find connection and host:

		shared_ptr<vector<string>> header_vec_ptr
			= string_split(*header, "\r\n");

		string _conn_value = get_header_value(header_vec_ptr, "proxy-connection");
		if(_conn_value.size()==0)
			_conn_value = get_header_value(header_vec_ptr, "connection");

		*host = get_header_value(header_vec_ptr, "host");


		//get to be removed field
		bool websocket_upgrade = false;
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
				else if (t == "upgrade") {
					if (string("websocket")
						!= get_header_value(header_vec_ptr, "upgrade")) {
						transform(t.begin(), t.end(), t.begin(), ::tolower);
						to_be_removed_header_list.emplace_back(t);
					}
					else {
						websocket_upgrade = true;
					}
				}
				else {
					transform(t.begin(), t.end(), t.begin(), ::tolower);
					to_be_removed_header_list.emplace_back(t);
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
			auto list_iter = to_be_removed_header_list.begin();
			while (list_iter != to_be_removed_header_list.end()) {
				if (temp.find(*list_iter) == 0) {
					auto temp_iter = list_iter;
					list_iter++;
					to_be_removed_header_list.erase(temp_iter);
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
			result->append("Connection: keep-alive");//默认keep-alive
		else
			result->append("Connection: close");

		if (websocket_upgrade) {
			result->append(", upgrade");
			_keep_alive = true;
			_conn_protocol = websocket_handshake;
		}

		result->append("\r\n");//header::connection end

		result->append("\r\n");//header end
		//append body
		result->append(*body);

		return _keep_alive;
	}

	



	

}