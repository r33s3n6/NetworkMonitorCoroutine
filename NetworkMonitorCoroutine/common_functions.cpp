#include "common_functions.h"
#include <algorithm>
using namespace std;
namespace common {
	using namespace proxy_tcp;


shared_ptr<vector<string>> string_split(const string& str, const string& pattern) {
	shared_ptr<vector<string>> res_vec(new vector<string>);
	if (str.size() == 0)
		return res_vec;
	size_t str_begin = 0;
	size_t pos = str.find(pattern);
	while (pos != string::npos) {
		res_vec->emplace_back(str.substr(str_begin, pos - str_begin));
		str_begin = pos + pattern.size();
		pos = str.find(pattern, str_begin);
	}
	res_vec->emplace_back(str.substr(str_begin, str.size() - str_begin));
	return res_vec;
}

size_t hex2decimal(const char* hex_str)
{
	char ch;
	size_t iret = 0;
	while (ch = *hex_str++)
	{
		iret = (iret << 4) | hex_table[ch];
	}
	return iret;
}




bool split_request(shared_ptr<const string> data, shared_ptr<string> header, shared_ptr<string> body)
{
	size_t header_end_pos = data->find("\r\n\r\n");
	if (header_end_pos == string::npos) {
		return false;
	}
	*header = data->substr(0, header_end_pos);
	*body = data->substr(header_end_pos + 4,
		data->size() - (header_end_pos + 4));
	return true;
}

//input lower case, header is case insensitive
string get_header_value(
	shared_ptr<const string> header_field, const string& header_name) {

	shared_ptr<vector<string>> header_vec_ptr
		= string_split(*header_field, "\r\n");

	for (auto header : *header_vec_ptr) {
		transform(header.begin(), header.end(), header.begin(), ::tolower);
		if (header.find(header_name) == 0) {
			return string_trim(header.substr(header_name.size()+1,
				header.size() - header_name.size()-1));//顺便去除冒号
		}
	}
	return "";
}

//input lower case, header is case insensitive
string get_header_value(
	const shared_ptr<vector<string>>& header_vec_ptr, const string& header_name) {

	for (auto header : *header_vec_ptr) {
		transform(header.begin(), header.end(), header.begin(), ::tolower);
		if (header.find(header_name) == 0) {
			return string_trim(header.substr(header_name.size() + 1,
				header.size() - header_name.size() - 1));//顺便去除冒号
		}
	}
	return "";
}


//仅仅根据头的信息分割报文，无法对内容完整性做出任何保证
proxy_tcp::integrity_status _http_integrity_check(shared_ptr<const string> http_data,size_t& split_pos)
{
	using namespace proxy_tcp;

	split_pos = http_data->size();
	size_t header_end_pos = http_data->find("\r\n\r\n");
	// check header integrity
	if (header_end_pos == string::npos) {//header not complete
		return integrity_status::wait;
	}
	// header integrity assured

	//determine body length
	size_t _request_length = 0;
	size_t length_pos = http_data->find("Content-Length:");
	size_t encoding_type_pos = http_data->find("Transfer-Encoding:");

	if (length_pos != string::npos) {// Content-Length exists
		for (int i = 0; i < 63; i++) {
			char _temp = (*http_data)[length_pos + 15 + i];
			if (_temp <= '9' &&
				_temp >= '0') {
				_request_length *= 10;
				_request_length += (_temp - '0');
			}
			else if (_temp == '\r')
				break;
		}
	}
	else if (encoding_type_pos != string::npos) { // Transfer-Encoding exists
		size_t _end = http_data->find("\r\n", encoding_type_pos);
		string t = http_data->substr(encoding_type_pos, _end - encoding_type_pos);
		if (t.find("chunked") != string::npos) {//
			split_pos = header_end_pos + 4;
			return integrity_status::chunked;
		}//else _request_length has been set to 0 by default

	}//else _request_length has been set to 0 by default

	//now _request_length is properly set

	//check whole request integrity
	if ((http_data->size() - (header_end_pos + 4)) < _request_length) {
		//request not complete
		
		return integrity_status::wait;
	}
	else{//几乎是完整的
		split_pos = _request_length + header_end_pos + 4; //若正好为size() 则正好完整，已在外部判断
		return integrity_status::intact;
	}


	// unexpected behaviour, return broken
	return integrity_status::broken;
}


proxy_tcp::integrity_status _chunked_integrity_check(shared_ptr<const string> http_data, size_t& split_pos)
{
	using namespace proxy_tcp;

	/*
* 结构
* [HEADER1]\r\n
* [HEADER2]\r\n
* \r\n
* [HEX_NUM]\r\n
* [DATA]\r\n
*/
	split_pos = http_data->size();

	size_t _chunk_length_end_pos =
		http_data->find("\r\n");

	if (_chunk_length_end_pos == string::npos)
		return integrity_status::wait_chunked;

	//get length
	size_t body_length = hex2decimal(http_data->substr(0, _chunk_length_end_pos).c_str());

	size_t body_end_pos = _chunk_length_end_pos + 2 + body_length + 2;

	split_pos = body_end_pos;//正好最后一个\r\n的后面一位

	if (http_data->size() < body_end_pos)
		return integrity_status::wait_chunked;

	//此时可能正好是一个完整的message，也有可能后面连缀着下一个报文的一些东西
	//因此先检查这个报文的完整性
	if ((*http_data)[body_end_pos - 2] == '\r' &&
		(*http_data)[body_end_pos - 1] == '\n') {//这个报文完整
		if (body_length == 0)
			return integrity_status::intact;//the last chunked message
		else
			return integrity_status::chunked;

	}
	else {
		return integrity_status::broken;
	}

	// unexpected behaviour, return broken
	return integrity_status::broken;

}

proxy_tcp::integrity_status _websocket_integrity_check(shared_ptr<const string> data, size_t& split_pos)
{
	split_pos = data->size();
	if (data->size() > 0)
		return integrity_status::websocket_intact;//todo
	else
		return integrity_status::wait;
}



shared_ptr<string> memory2hex_string(shared_ptr<const string> data)
{
	shared_ptr<string> res(new string(""));
	for (unsigned char _byte : *data) {

		res->push_back(hex_char[_byte >> 4]);
		res->push_back(hex_char[_byte & 0b00001111]);

		res->push_back(' ');
	}

	res->push_back('\n');

	

	return res;
}

request_type _get_request_type(const string& data)
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


}

