
#include "proxy_handler.h"

#include "common_functions.h"
#include <algorithm>
#include <list>
using namespace common;

namespace proxy_server {

	



	http_proxy_handler::http_proxy_handler(
		breakpoint_manager& bp_mgr,
		display_filter& disp_fil,
		shared_ptr<client_unit> _client)
		: _breakpoint_manager(bp_mgr),
		_display_filter(disp_fil),
		_client(_client)
	{
	}

	connection_behaviour http_proxy_handler::handle_error(shared_ptr<string> result)//TODO
	{
		*result = "HTTP/1.1 400 Bad Request\r\n\r\n";

		return respond_and_close;
	}

	awaitable<connection_behaviour> http_proxy_handler::send_message(shared_ptr<string> msg, bool with_ssl, bool force_old_conn)
	{
		host.reset(new string(""));


		//非connect
		shared_ptr<string> _modified_data(new string(""));
		bool _keep_alive = _process_header(msg, _modified_data);//在其中host被修改

		if (_modified_data->size() == 0) {
			co_return respond_error;
		}


		if (_update_id == -2) {//说明是一次全新的请求，不是某次chunked的后续
			if (_breakpoint_manager.check(*_modified_data)) {
				//断点
				//_modified_data 在此处是可能被修改的
				_update_id = co_await _display_filter.display_breakpoint_req(_modified_data);
				if (_update_id == -1) {//不继续执行
					_update_id = -2;//reset to default
					co_return connection_behaviour::ignore;
				}
			}
			else {//新显示 
				_update_id = _display_filter.display(_modified_data);
			}
		}
		else {//是某次的后续，因此直接更新
			_display_filter.update_display_req(_update_id, _modified_data);
		}

		connection_behaviour _behaviour;



		_behaviour = co_await _client->send_request(*host, *_modified_data, with_ssl, force_old_conn);

		if (_behaviour == respond_error) {
			_display_filter.update_display_error(_update_id, make_shared<string>("sending message fucked up"));//TODO
			co_return _behaviour;
		}
		else {
			if (_keep_alive)
				co_return respond_and_keep_alive;
			else
				co_return respond_and_close;

		}



		
	}

	awaitable<connection_behaviour> http_proxy_handler::receive_message(shared_ptr<string>& rsp, bool with_ssl)
	{
		//必然有update_id
		//assert(update_id>=0)
		connection_behaviour _behaviour;
		_behaviour = co_await _client->receive_response(rsp);
		if (_behaviour == respond_error) {

			_display_filter.update_display_error(_update_id, rsp);
			co_return _behaviour;
		}

		if (_breakpoint_manager.check(*rsp)) {
			//断点
			if (-1 == co_await _display_filter.display_breakpoint_rsp(_update_id, rsp)) {//result 在此处是可能被修改的
				//请求被阻断
				_update_id = -2;
				co_return connection_behaviour::ignore;
			}
			//else 不用做任何事，因为display_breakpoint_rsp 已经处理好了后续显示工作

		}
		else {//手动更新显示
			_display_filter.update_display_rsp(_update_id, rsp);
		}

		
		if(_behaviour != keep_receiving_data){
			//reset _update_id
			_update_id = -2;
		}

		co_return _behaviour;
		
	}



	//=====================common functions=============================

	


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
			"upgrade",
			"accept-encoding"//TODO: 解决gzip, br, deflate后再加回去吧
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