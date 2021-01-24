/*
* main.cpp
* 
* Main Entry point
* 
* Distributed under the Boost Software License, Version 1.0.
(See accompanying file ./License/LICENSE_1_0.txt or copy at
https://www.boost.org/LICENSE_1_0.txt)

*/


#include <iostream>
#include <cstdio>
using namespace std;

#include <boost/asio.hpp>
#include <boost/thread/thread.hpp>
#include <boost/bind/bind.hpp>
#include <boost/array.hpp>

#include <boost/lexical_cast.hpp>


using boost::asio::ip::tcp;

//#include "TestServer.h"
#include "proxy_server.h"

/*未完成的部分:
* 
* logging 类
* 
* 
* 
* client_unit::verify
* 
* 
* 
* 
* 
* certificate_manager::
create_ca

auto_trust_ca
* 
* 
*/
// client_unit send request 改成 connection:close 一直尝试读取，读一点写一点，直到读到eof

/*
程序入口点
作为界面程序的后台处理消息循环
可以单独调试
*/

int main(int argc, char* argv[])
{
    try
    {
        // Check command line arguments.
        if (argc != 4)
        {
            std::cerr << "Usage: proxy_server <address> <port> <threads>\n";
            std::cerr << "  For IPv4, try:\n";
            std::cerr << "    proxy_server 0.0.0.0 5559 16\n";
            std::cerr << "  For IPv6, try:\n";
            std::cerr << "    proxy_server 0::0 5559 16\n";
            return 1;
        }
        
        // Initialise the server.
        std::size_t num_threads = boost::lexical_cast<std::size_t>(argv[3]);
        proxy_server::proxy_server srv(argv[1], argv[2], num_threads);
        printf("listening on %s:%s\n", argv[1], argv[2]);

        // Run the server until stopped.
        srv.start();
    }
    catch (std::exception& e)
    {
        std::cerr << "exception: " << e.what() << "\n";
    }
    

	return 0;
}