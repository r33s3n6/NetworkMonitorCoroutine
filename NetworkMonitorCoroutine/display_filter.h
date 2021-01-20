#pragma once

#include <string>
#include <memory>
#include <functional>

using namespace std;


#include <boost/asio.hpp>

namespace proxy_server{

	using boost::asio::ip::tcp;
	using boost::asio::awaitable;
	using boost::asio::co_spawn;
	using boost::asio::detached;
	using boost::asio::redirect_error;
	using boost::asio::use_awaitable;



class display_filter
{
public:
	void display(shared_ptr<string> req_data, shared_ptr<string> rsp_data);

	void _temp_display(const shared_ptr<string>& data);

	int display(const shared_ptr<string>& req_data);

	awaitable<int> display_breakpoint_req(shared_ptr<string> req_data);//里面要显示出来
	void update_display(int id, const shared_ptr<string>& rsp_data);
	awaitable<int> display_breakpoint_rsp(shared_ptr<string> req_data, shared_ptr<string> rsp_data, int update_id);

	//里面要显示出来
};

}


