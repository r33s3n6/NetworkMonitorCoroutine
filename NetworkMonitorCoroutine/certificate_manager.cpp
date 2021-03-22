#include "certificate_manager.h"


/*
Copyright (c) 2017, 2018, 2019 Linus Karlsson

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#include <stdint.h>
#include <stdio.h>
#include <stdexcept>
#include <iostream>
using namespace std;

#define RSA_KEY_BITS (2048)

#define REQ_DN_C "CN"
#define REQ_DN_ST "Beijing"
#define REQ_DN_L "Beijing"
#define REQ_DN_O "DO_NOT_TRUST_DEV_CA"
#define REQ_DN_OU "DO_NOT_TRUST_DEV_CA"
//#define REQ_DN_CN "DO_NOT_TRUST_DEV_CA"

static void crt_to_pem(X509* crt, uint8_t** crt_bytes, size_t* crt_size);
static int generate_key_csr(EVP_PKEY** key, X509_REQ** req, const char* CN_info);
static int generate_set_random_serial(X509* crt);
static int generate_signed_key_pair(EVP_PKEY* ca_key, X509* ca_crt, EVP_PKEY** key, X509** crt, const char* SAN_info);
static void key_to_pem(EVP_PKEY* key, uint8_t** key_bytes, size_t* key_size);
static int load_ca(const char* ca_key_path, EVP_PKEY** ca_key, const char* ca_crt_path, X509** ca_crt);
static void print_bytes(uint8_t* data, size_t size);
static int add_ext(X509* cert, int nid, const char* value);
static int make_ca_cert(X509** x509p, EVP_PKEY** pkeyp, int bits, int days);


void crt_to_pem(X509* crt, uint8_t** crt_bytes, size_t* crt_size)
{
	/* Convert signed certificate to PEM format. */
	BIO* bio = BIO_new(BIO_s_mem());
	PEM_write_bio_X509(bio, crt);
	*crt_size = BIO_pending(bio);
	*crt_bytes = (uint8_t*)malloc(*crt_size + 1);
	BIO_read(bio, *crt_bytes, *crt_size);
	BIO_free_all(bio);
}



int generate_signed_key_pair(EVP_PKEY* ca_key, X509* ca_crt, EVP_PKEY** key, X509** crt, const char * SAN_info)
{
	X509_REQ* req = NULL;
	try
	{
		/* Generate the private key and corresponding CSR. */
		
		if (!generate_key_csr(key, &req,SAN_info)) {
			fprintf(stderr, "Failed to generate key and/or CSR!\n");
			return 0;
		}

		/* Sign with the CA. */
		*crt = X509_new();
		if (!*crt) {
			throw runtime_error("generate X509 certificate failed");
		}

		X509_set_version(*crt, 2); /* Set version to X509v3 */

		/* Generate random 20 byte serial. */
		if (!generate_set_random_serial(*crt)) 
			throw runtime_error("generate_set_random_serial failed");

		/* Set issuer to CA's subject. */

		X509_set_issuer_name(*crt, X509_get_subject_name(ca_crt));

		/* Set validity of certificate to 10 years. */
		X509_gmtime_adj(X509_get_notBefore(*crt), 0);
		X509_gmtime_adj(X509_get_notAfter(*crt), (long)10 * 365 * 24 * 3600);

		/* Get the request's subject and just use it (we don't bother checking it since we generated
		 * it ourself). Also take the request's public key. */
		X509_set_subject_name(*crt, X509_REQ_get_subject_name(req));
		EVP_PKEY* req_pubkey = X509_REQ_get_pubkey(req);
		X509_set_pubkey(*crt, req_pubkey);
		EVP_PKEY_free(req_pubkey);
		string temp("DNS:");
		temp.append(SAN_info);
		add_ext(*crt, NID_subject_alt_name, temp.c_str());
		/* Now perform the actual signing with the CA. */
		if (X509_sign(*crt, ca_key, EVP_sha256()) == 0) 
			throw runtime_error("X509_sign failed");

		X509_REQ_free(req);
		return 1;

	}
	catch (const std::exception& e)
	{
		cerr << e.what() << endl;
		EVP_PKEY_free(*key);
		X509_REQ_free(req);
		X509_free(*crt);
		return 0;
	}

	
}

int generate_key_csr(EVP_PKEY** key, X509_REQ** req,const char * CN_info)
{
	*key = NULL;
	*req = NULL;
	RSA* rsa = NULL;
	BIGNUM* e = NULL;
	try
	{
		*key = EVP_PKEY_new();
		if (!*key) throw runtime_error("EVP_PKEY_new failed");
		*req = X509_REQ_new();
		if (!*req) throw runtime_error("X509_REQ_new failed");
		rsa = RSA_new();
		if (!rsa) throw runtime_error("RSA_new failed");
		e = BN_new();
		if (!e) throw runtime_error("BN_new failed");

		BN_set_word(e, 65537);
		if (!RSA_generate_key_ex(rsa, RSA_KEY_BITS, e, NULL)) 
			throw runtime_error("RSA_generate_key_ex failed");
		if (!EVP_PKEY_assign_RSA(*key, rsa)) 
			throw runtime_error("EVP_PKEY_assign_RSA failed");



		X509_REQ_set_pubkey(*req, *key);

		/* Set the DN of the request. */
		X509_NAME* name = X509_REQ_get_subject_name(*req);
		X509_NAME_add_entry_by_txt(name, "C", MBSTRING_ASC, (const unsigned char*)REQ_DN_C, -1, -1, 0);
		X509_NAME_add_entry_by_txt(name, "ST", MBSTRING_ASC, (const unsigned char*)REQ_DN_ST, -1, -1, 0);
		X509_NAME_add_entry_by_txt(name, "L", MBSTRING_ASC, (const unsigned char*)REQ_DN_L, -1, -1, 0);
		X509_NAME_add_entry_by_txt(name, "O", MBSTRING_ASC, (const unsigned char*)REQ_DN_O, -1, -1, 0);
		X509_NAME_add_entry_by_txt(name, "OU", MBSTRING_ASC, (const unsigned char*)REQ_DN_OU, -1, -1, 0);
		X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, (const unsigned char*)CN_info, -1, -1, 0);

		/* Self-sign the request to prove that we possess the key. */
		if (!X509_REQ_sign(*req, *key, EVP_sha256())) 
			throw runtime_error("X509_REQ_sign failed");

		BN_free(e);

		return 1;
	}
	catch (const std::exception& _e)
	{
		cerr << _e.what() << endl;
		EVP_PKEY_free(*key);
		X509_REQ_free(*req);
		RSA_free(rsa);
		BN_free(e);
		return 0;
	}
	


	
}

int generate_set_random_serial(X509* crt)
{
	/* Generates a 20 byte random serial number and sets in certificate. */
	unsigned char serial_bytes[20];
	if (RAND_bytes(serial_bytes, sizeof(serial_bytes)) != 1) return 0;
	serial_bytes[0] &= 0x7f; /* Ensure positive serial! */
	BIGNUM* bn = BN_new();
	BN_bin2bn(serial_bytes, sizeof(serial_bytes), bn);
	ASN1_INTEGER* serial = ASN1_INTEGER_new();
	BN_to_ASN1_INTEGER(bn, serial);

	X509_set_serialNumber(crt, serial); // Set serial.

	ASN1_INTEGER_free(serial);
	BN_free(bn);
	return 1;
}

void key_to_pem(EVP_PKEY* key, uint8_t** key_bytes, size_t* key_size)
{
	/* Convert private key to PEM format. */
	BIO* bio = BIO_new(BIO_s_mem());
	PEM_write_bio_PrivateKey(bio, key, NULL, NULL, 0, NULL, NULL);
	*key_size = BIO_pending(bio);
	*key_bytes = (uint8_t*)malloc(*key_size + 1);
	BIO_read(bio, *key_bytes, *key_size);
	BIO_free_all(bio);
}

int load_ca(const char* ca_key_path, EVP_PKEY** ca_key, const char* ca_crt_path, X509** ca_crt)
{
	BIO* bio = NULL;
	*ca_crt = NULL;
	*ca_key = NULL;

	/* Load CA public key. */
	bio = BIO_new(BIO_s_file());
	if (!BIO_read_filename(bio, ca_crt_path)) goto err;
	*ca_crt = PEM_read_bio_X509(bio, NULL, NULL, NULL);
	if (!*ca_crt) goto err;
	BIO_free_all(bio);

	/* Load CA private key. */
	bio = BIO_new(BIO_s_file());
	if (!BIO_read_filename(bio, ca_key_path)) goto err;
	*ca_key = PEM_read_bio_PrivateKey(bio, NULL, NULL, NULL);
	if (!ca_key) goto err;
	BIO_free_all(bio);
	return 1;
err:
	BIO_free_all(bio);
	X509_free(*ca_crt);
	EVP_PKEY_free(*ca_key);
	return 0;
}

void print_bytes(uint8_t* data, size_t size)
{
	for (size_t i = 0; i < size; i++) {
		printf("%c", data[i]);
	}
}

int add_ext(X509* cert, int nid, const char* value)
{
	X509_EXTENSION* ex;
	X509V3_CTX ctx;
	/* This sets the 'context' of the extensions. */
	/* No configuration database */
	X509V3_set_ctx_nodb(&ctx);
	/* Issuer and subject certs: both the target since it is self signed,
	 * no request and no CRL
	 */
	X509V3_set_ctx(&ctx, cert, cert, NULL, NULL, 0);
	ex = X509V3_EXT_conf_nid(NULL, &ctx, nid, value);
	if (!ex)
		return 0;

	X509_add_ext(cert, ex, -1);
	X509_EXTENSION_free(ex);
	return 1;
}

//=======================================================================================

certificate_manager::~certificate_manager() {
	/* Free stuff. */
	if (ca_key)
		EVP_PKEY_free(ca_key);
	if (ca_crt)
		X509_free(ca_crt);


}

void certificate_manager::create_root_ca(const string& cert_path, const string& key_path)
{

	X509* x509 = NULL;
	EVP_PKEY* pkey = NULL;


	make_ca_cert(&x509, &pkey, RSA_KEY_BITS, 3650);

	FILE* fp_cert = fopen(cert_path.c_str(), "w+");
	FILE* fp_key = fopen(key_path.c_str(), "w+");
	PEM_write_PrivateKey(fp_key, pkey, NULL, NULL, 0, NULL, NULL);
	PEM_write_X509(fp_cert, x509);
	fclose(fp_cert);
	fclose(fp_key);
	X509_free(x509);
	EVP_PKEY_free(pkey);


//TODO: https://opensource.apple.com/source/OpenSSL/OpenSSL-22/openssl/demos/x509/mkcert.c
}

void certificate_manager::create_server_certificate(const string& ca_path, const string& domain, const string& cert_path)
{

}

shared_ptr<cert_key> certificate_manager::get_server_certificate(const string& domain) 
{
	if (cached_cert.find(domain) == cached_cert.end()) {
		auto new_cert = create_server_certificate(domain);
		cached_cert[domain] = new_cert;
		return new_cert;
	}
	else {
		return cached_cert[domain];
	}
		

}

shared_ptr<cert_key> certificate_manager::create_server_certificate(const string& domain)
{
	/* Generate keypair */
	EVP_PKEY* key = NULL;
	X509* crt = NULL;

	int ret = generate_signed_key_pair(ca_key, ca_crt, &key, &crt,domain.c_str());
	if (!ret) {
		fprintf(stderr, "Failed to generate key pair!\n");
		return make_shared<cert_key>();
	}
	/* Convert key and certificate to PEM format. */
	uint8_t* key_bytes = NULL;
	uint8_t* crt_bytes = NULL;
	size_t key_size = 0;
	size_t crt_size = 0;

	key_to_pem(key, &key_bytes, &key_size);
	crt_to_pem(crt, &crt_bytes, &crt_size);

	//DEBUG
	//print_bytes(key_bytes, key_size);
	//print_bytes(crt_bytes, crt_size);

	EVP_PKEY_free(key);
	X509_free(crt);

	return make_shared<cert_key>(key_bytes, crt_bytes);
}



bool certificate_manager::read_root_ca(const string& ca_crt_path, const string& ca_key_path)
{
	/* Load CA key and cert. */
	int retry = 2;
	while (!load_ca(ca_key_path.c_str(), &ca_key, ca_crt_path.c_str(), &ca_crt)) {
		create_root_ca(ca_crt_path, ca_key_path);
		retry--;
		if (retry < 0) {
			fprintf(stderr, "Failed to load CA certificate and/or key!\n");
			return false;
		}
	}
	return true;
}

int make_ca_cert(X509** x509p, EVP_PKEY** key, int bits, int days)
{
	X509* x;
	//EVP_PKEY* pk;
	RSA* rsa;
	X509_NAME* name = NULL;
	BIGNUM* e = NULL;
	try {
		*key = EVP_PKEY_new();
		if (!*key) throw runtime_error("EVP_PKEY_new failed");
		x = X509_new();
		if (!x) throw runtime_error("X509_new failed");
		rsa = RSA_new();
		if (!rsa) throw runtime_error("RSA_new failed");
		e = BN_new();
		if (!e) throw runtime_error("BN_new failed");

		BN_set_word(e, 65537);
		if (!RSA_generate_key_ex(rsa, RSA_KEY_BITS, e, NULL))
			throw runtime_error("RSA_generate_key_ex failed");
		if (!EVP_PKEY_assign_RSA(*key, rsa))
			throw runtime_error("EVP_PKEY_assign_RSA failed");

		X509_set_pubkey(x, *key);

		name = X509_get_subject_name(x);

		/* This function creates and adds the entry, working out the
		 * correct string type and performing checks on its length.
		 * Normally we'd check the return value for errors...
		 */

		X509_NAME_add_entry_by_txt(name, "C", MBSTRING_ASC, (const unsigned char*)REQ_DN_C, -1, -1, 0);
		X509_NAME_add_entry_by_txt(name, "ST", MBSTRING_ASC, (const unsigned char*)REQ_DN_ST, -1, -1, 0);
		X509_NAME_add_entry_by_txt(name, "L", MBSTRING_ASC, (const unsigned char*)REQ_DN_L, -1, -1, 0);
		X509_NAME_add_entry_by_txt(name, "O", MBSTRING_ASC, (const unsigned char*)REQ_DN_O, -1, -1, 0);
		X509_NAME_add_entry_by_txt(name, "OU", MBSTRING_ASC, (const unsigned char*)REQ_DN_OU, -1, -1, 0);
		X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, (const unsigned char*)REQ_DN_O, -1, -1, 0);

		BN_free(e);

		X509_set_version(x, 2);

		if (!generate_set_random_serial(x))
			throw runtime_error("generate_set_random_serial failed");

		X509_set_issuer_name(x, name);

		X509_gmtime_adj(X509_get_notBefore(x), 0);
		X509_gmtime_adj(X509_get_notAfter(x), (long)60 * 60 * 24 * days);

		
		add_ext(x, NID_basic_constraints, "critical,CA:TRUE");

		if (!X509_sign(x, *key, EVP_sha256()))
			throw runtime_error("X509_sign failed");

	}
	catch (const std::exception& _e)
	{
		cerr << _e.what() << endl;
		EVP_PKEY_free(*key);
		X509_free(x);
		RSA_free(rsa);
		BN_free(e);
		return 0;
	}

	*x509p = x;

	return 1;
}