#include "breakpoint_manager.h"

#include "common_functions.h"
using namespace common;


bool breakpoint_manager::check(shared_ptr<const session_info> _session_info, bool is_request)
{
    //TODO
    const breakpoint_filter& filter = is_request ? req_filter : rsp_filter;

    if (!filter.enable_breakpoint) {
        return false;
    }

	shared_ptr<string> header(new string(""));
	shared_ptr<string> body(new string(""));
    
    /*
	if (!split_request(
		is_request?
		_session_info->raw_req_data:
		_session_info->raw_rsp_data, header, body))
		return false;
        */
    if (!split_request(_session_info->raw_req_data , header, body))//TODO:目前只查找请求头
        return false;
	shared_ptr<vector<string>> header_vec_ptr
		= string_split(*header, "\r\n");



    if (filter.header_filter_vec.empty())
        return false;


    for (auto header : filter.header_filter_vec) {
        
        if (header.value.size() == 0)
            continue;

        for (auto value_filter : header.value) {//对每一个value_filter都查找，性能有较大消耗

            shared_ptr<vector<string>> keywords = string_split(value_filter, "*");
            if (keywords->size() == 0)
                continue;
            string value = get_header_value(header_vec_ptr, header.key);


            int last_pos = -1;

            bool success = true;

            //TODO:正则表达式，高效实现*
            for (int i = 0; i < keywords->size(); i++) {//*的简单实现
                if ((*keywords)[i].size() == 0)
                    continue;
                size_t pos = value.find((*keywords)[i]);
                
                if (pos != string::npos && ((int)pos > last_pos)) {
                    last_pos = (int)pos;
                    continue;
                }
                else {
                    success = false;
                    break;
                }

            }
            if (success)
                return true;

        }
        

    }

       



    return false;
}
