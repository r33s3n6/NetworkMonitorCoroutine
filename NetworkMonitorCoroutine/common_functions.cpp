#include "common_functions.h"
#include <algorithm>
using namespace std;
namespace common {



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




bool split_request(const shared_ptr<string>& data, shared_ptr<string> header, shared_ptr<string> body)
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
	const shared_ptr<string>& header_field, const string& header_name) {

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

proxy_server::integrity_status _http_integrity_check(const shared_ptr<string>& _whole_request,size_t& split_pos)
{
	using namespace proxy_server;
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
			if ((*temp_vec_ptr)[1].size() == body_length) {
				if (body_length == 0)
					return integrity_status::success;
				else
					return integrity_status::wait_chunked;
			}
				

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


shared_ptr<string> memory2hex_string(const shared_ptr<string>& data)
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


}

