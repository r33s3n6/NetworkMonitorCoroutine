#include "ssl_layer.h"

namespace proxy_server {



awaitable<bool> ssl_layer::_do_handshake(const string& host)
{
    co_return true;
}

awaitable<void> ssl_layer::decrypt_append(shared_ptr<string> str, const char* data, size_t length)
{
    co_return;
}

awaitable<shared_ptr<string>> ssl_layer::ssl_encrypt(const string& data)
{
    co_return make_shared<string>("DEBUG");
}

}