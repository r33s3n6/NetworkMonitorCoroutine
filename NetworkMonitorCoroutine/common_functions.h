#pragma once
#include <string>
#include <vector>
#include <memory>
using namespace std;

#include "connection_enums.h"

namespace common {



    shared_ptr<vector<string>> string_split(const string& str, const string& pattern);

static size_t hex_table[] = {
    0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,
    0,0,0,0,1,2,3,4,5,6,
    7,8,9,0,0,0,0,0,0,
    0,10,11,12,13,14,15,0,0,
    0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,10,
    11,12,13,14,15 };

size_t hex2decimal(const char* hex_str);

inline string string_trim(const string& original)
{
    size_t _begin = original.find_first_not_of(" ");
    size_t _size = original.find_last_not_of(" ") - _begin + 1;
    return original.substr(_begin, _size);
}

inline string string_trim(const string&& original)
{
    size_t _begin = original.find_first_not_of(" ");
    size_t _size = original.find_last_not_of(" ") - _begin + 1;
    return original.substr(_begin, _size);
}

bool split_request(shared_ptr<const string> data, shared_ptr<string> header, shared_ptr<string> body);


string get_header_value(
    shared_ptr<const string> header_field, const string& header_name);

string get_header_value(
    const shared_ptr<vector<string>>& header_field, const string& header_name);

proxy_tcp::integrity_status _http_integrity_check(
    shared_ptr<const string> _whole_request,  size_t& split_pos);

proxy_tcp::integrity_status _chunked_integrity_check(shared_ptr<const string> http_data, size_t& split_pos);

static const char hex_char[] {"0123456789ABCDEF"};
shared_ptr<string> memory2hex_string(shared_ptr<const string> data);

proxy_tcp::request_type _get_request_type(const string& data);

}