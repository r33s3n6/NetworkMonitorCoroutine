#include "../NetworkMonitorCoroutine/display_filter.h"

#include <cstdio>
#include <iostream>
using namespace std;

#include "../NetworkMonitorCoroutine/common_functions.h"
using namespace common;

namespace proxy_tcp {

	

	void display_filter::display(shared_ptr<session_info> _session_info)
	{


		emit session_created(_session_info);
	}



	void display_filter::update_display_req(
		shared_ptr<session_info> _session_info)
	{

		emit session_req_updated(_session_info);
	}

	void display_filter::display_rsp(
		shared_ptr<session_info> _session_info)
	{


		emit session_rsp_begin(_session_info);

	}

	void display_filter::update_display_rsp(
		shared_ptr<session_info> _session_info)
	{


		emit session_rsp_updated(_session_info);

	}

	void display_filter::update_display_error(
		shared_ptr<session_info> _session_info)
	{
		emit session_error(_session_info);

	}

	







}
