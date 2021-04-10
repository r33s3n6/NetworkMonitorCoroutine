#include "../NetworkMonitorCoroutine/display_filter.h"

#include <cstdio>
#include <iostream>
using namespace std;

#include "../NetworkMonitorCoroutine/common_functions.h"
using namespace common;

namespace proxy_tcp {



	inline bool display_filter::is_filtered(shared_ptr<session_info> _session_info) {

		if (filter && (!filter->empty()))
			if(_session_info->raw_req_data)//TODO: 不应该出现这种情况，但是调试的时候发现了
				return header_check(*_session_info->raw_req_data, *filter);
		else
			return false;

		return false;
	}

	void display_filter::display(shared_ptr<session_info> _session_info)
	{

		if (is_filtered(_session_info)) {
			_session_info->forever_filtered = true;
		}
		else {
			emit session_created(_session_info);
		}
			

			

	}



	void display_filter::update_display_req(
		shared_ptr<session_info> _session_info)
	{
		if (!_session_info->forever_filtered)
			emit session_req_updated(_session_info);

	}

	void display_filter::display_filter::complete_req(shared_ptr<session_info> _session_info)
	{

		if (!_session_info->forever_filtered)
			emit session_req_completed(_session_info);
	}

	void display_filter::display_rsp(
		shared_ptr<session_info> _session_info)
	{

		if (!_session_info->forever_filtered)
			emit session_rsp_begin(_session_info);

	}

	void display_filter::update_display_rsp(
		shared_ptr<session_info> _session_info)
	{

		if (!_session_info->forever_filtered)
			emit session_rsp_updated(_session_info);

	}

	void display_filter::display_filter::complete_rsp(shared_ptr<session_info> _session_info)
	{
		if (!_session_info->forever_filtered)
			emit session_rsp_completed(_session_info);
	}

	void display_filter::update_display_error(
		shared_ptr<session_info> _session_info)
	{
		if (!_session_info->forever_filtered)
			emit session_error(_session_info);

	}

	







}
