#include "../NetworkMonitorCoroutine/display_filter.h"

#include <cstdio>
#include <iostream>
using namespace std;

#include "../NetworkMonitorCoroutine/common_functions.h"
using namespace common;

namespace proxy_tcp {

	

	int display_filter::display(shared_ptr<string> req_data) //return update_id
	{
		size_t temp_id = get_temp_id();

		emit session_created(req_data, temp_id);
		

		return temp_id;
	}




	void display_filter::update_display_req(int id, shared_ptr<string> req_data)
	{

		emit session_req_updated(req_data, id);
	}

	void display_filter::update_display_rsp(int id, shared_ptr<string> rsp_data)
	{
		emit session_rsp_updated(rsp_data, id);

	}

	void display_filter::update_display_error(int id, shared_ptr<string> rsp_data)
	{
		emit session_error(rsp_data, id);

	}

	//return update_id/-1(ignore)
	awaitable<int> display_filter::display_breakpoint_req(shared_ptr<string> req_data)
	{
		size_t temp_id = get_temp_id();

		emit session_created_breakpoint(req_data, temp_id);


		//co_await定时器检测是否有信号
		if (false)
			co_return -1;
		
		co_return temp_id;
	}

	awaitable<int> display_filter::display_breakpoint_rsp(int update_id,
		shared_ptr<string> rsp_data)//更新旧显示
	{
		
		size_t temp_id = get_temp_id();
		emit session_rsp_updated_breakpoint(rsp_data, temp_id);

		
		if (false)
			co_return -1;
		co_return temp_id;
	}





}
