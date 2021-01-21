#pragma once

#include <string>
using namespace std;

class certificate_manager
{
public:

	certificate_manager(const certificate_manager&) = delete;
	certificate_manager& operator=(const certificate_manager&) = delete;

	//certificate_manager(const string& filename);
	certificate_manager();

	void create_root_ca(const string& path);
	void create_server_certificate(const string& ca_path, const string& domain, const string& cert_path);


private:
	//ca_file

};

