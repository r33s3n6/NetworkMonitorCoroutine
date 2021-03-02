#pragma once
#include <string>
#include <vector>
#include <map>
#include <typeinfo>
using namespace std;

#define    NAME(name)    (#name)

/*
template <typename T>
static string type_name(T x) {
	if (typeid(x) == typeid(string)) {
		return "string";
	}
	else if(){

	}
	switch (typeid(x)) {
	case typeid(string):
		
	case typeid(size_t):
		return "size_t";
	case typeid(bool):
		return "bool";

	//case typeid(bool).name():
	//	return "bool";



	}
}*/
#define	ADD_ENTRY(x) conf_entry_map[NAME(x)]=conf_entry(typeid(x).name(),&x);



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
/*
enum value_type {
	BOOL,STRING,INT_ARRAY,SIZE_T
};*/

struct conf_entry {
	//string key;
	string type;
	void* address;
	conf_entry(string type,void* address):type(type),address(address){}
	conf_entry():address(nullptr){}
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
	config(string path = "config.dat") { 
		_setup_conf_entry_vec();
		load_config_from_file(path); }
private:
	map<string, conf_entry> conf_entry_map;
	//vector<conf_entry> conf_entry_vec;
	void _setup_conf_entry_vec() {
		
		//typeid(system_proxy).;
		//conf_entry_vec.clear();
		conf_entry_map.clear();
		ADD_ENTRY(ssl_decrypt);
		ADD_ENTRY(system_proxy);
		ADD_ENTRY(allow_lan_conn);
		ADD_ENTRY(verify_server_certificate);

		ADD_ENTRY(req_filter.enable_breakpoint);
		ADD_ENTRY(rsp_filter.enable_breakpoint);

		ADD_ENTRY(req_filter.raw_custom_header_filter);
		ADD_ENTRY(rsp_filter.raw_custom_header_filter);

		ADD_ENTRY(req_filter.raw_host_filter);
		ADD_ENTRY(rsp_filter.raw_host_filter);

		ADD_ENTRY(io_context_pool_size);

		ADD_ENTRY(secondary_proxy);
		ADD_ENTRY(port);

		ADD_ENTRY(column_width);


		/*
		conf_entry_vec = vector<conf_entry>({
			{"ssl_decrypt",BOOL,&ssl_decrypt},
			{"system_proxy",BOOL,&system_proxy},
			{"allow_lan_conn",BOOL,&allow_lan_conn},
			{"verify_server_certificate",BOOL,&verify_server_certificate},
			{"enable_breakpoint_req",BOOL,&req_filter.enable_breakpoint},
			{"enable_breakpoint_rsp",BOOL,&rsp_filter.enable_breakpoint},

			{"raw_custom_header_filter_req",STRING,&req_filter.raw_custom_header_filter},
			{"raw_custom_header_filter_rsp",STRING,&rsp_filter.raw_custom_header_filter},

			{"raw_host_filter_req",STRING,&req_filter.raw_host_filter},
			{"raw_host_filter_rsp",STRING,&rsp_filter.raw_host_filter},

			{"io_context_pool_size",SIZE_T,&io_context_pool_size},
			});*/
	}



};

