#pragma once
#include <string>
#include <vector>
using namespace std;

static const char* _table_header_name[] = {
	"#", "URL","Code","Protocol","Host","Body","Content-Type"
};


struct http_header {
    string key;//case insensitive
    vector<string> value;//case sensitive
};

struct breakpoint_filter {
    vector<http_header> header_filter_vec;

    string raw_custom_header_filter;
    string raw_host_filter;

    bool enable_breakpoint = false;

};


class config
{
public:
	breakpoint_filter req_filter;
	breakpoint_filter rsp_filter;

	int column_width[sizeof(_table_header_name) / sizeof(const char*)] = {
	20,200,35,45,200,40,70
	};

	bool ssl_decrypt = true;//TODO
	bool system_proxy = false;//TODO
	bool allow_lan_conn = true;
	bool verify_server_certificate = true;
	size_t io_context_pool_size = 16;
	string secondary_proxy;//TODO
	string port="5559";


	void load_config_from_file(string path = "config.dat");//TODO
	void save_config(string path = "config.dat");//TODO
	config(string path = "config.dat") { load_config_from_file(path); }

};

