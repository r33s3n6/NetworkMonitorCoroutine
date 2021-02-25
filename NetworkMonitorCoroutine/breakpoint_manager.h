#pragma once
#include <string>
#include <memory>
using namespace std;

#include "../QTFrontend/SessionDataModel.h"


struct breakpoint_filter {
    shared_ptr<map<string, string>> header_filter;

    string raw_custom_header_filter;
    string raw_host_filter;

    bool enable_breakpoint = false;
};


class breakpoint_manager
{
public:
	bool check(shared_ptr<const session_info> _session_info,bool is_request);
    breakpoint_manager(breakpoint_filter& req, breakpoint_filter& rsp): req_filter(req),rsp_filter(rsp) {}
private:
    const breakpoint_filter& req_filter;
    const breakpoint_filter& rsp_filter;
};

