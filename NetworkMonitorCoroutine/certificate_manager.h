#pragma once

#include <string>
#include <memory>
using namespace std;

#include <openssl/err.h>
#include <openssl/conf.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
#include <openssl/x509v3.h>

#include <unordered_map>
struct cert_key {
	string key_bytes;
	string crt_bytes;

	cert_key(uint8_t* key_bytes = nullptr, uint8_t* crt_bytes = nullptr) :
		key_bytes((char*)key_bytes), crt_bytes((char*)crt_bytes) {
		free(key_bytes);
		free(crt_bytes);
	}

};

//TODO: generate CA certificate
class certificate_manager
{
public:

	certificate_manager(const certificate_manager&) = delete;
	certificate_manager& operator=(const certificate_manager&) = delete;

	certificate_manager(const string& ca_cert, const string& ca_key) {
		read_root_ca(ca_cert, ca_key);
	}
	certificate_manager() {};

	~certificate_manager();

	void create_root_ca(const string& cert_path, const string& key_path);

	void create_server_certificate(const string& ca_path, const string& domain, const string& cert_path);
	shared_ptr<cert_key> get_server_certificate(const string& domain);
	
	bool read_root_ca(const string& ca_crt_path, const string& ca_key_path);
	

private:
	unordered_map<string, shared_ptr<cert_key>> cached_cert;

	EVP_PKEY* ca_key = NULL;
	X509* ca_crt = NULL;
	//ca_buffer
	shared_ptr<cert_key> create_server_certificate(const string& domain);

};



