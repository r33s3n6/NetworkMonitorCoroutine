#pragma once
#include <string>
#include <memory>
#include <list>
using namespace std;

#include "../QTFrontend/SessionDataModel.h"

#include "config.h"



class breakpoint_manager
{
public:
	bool check(shared_ptr<const session_info> _session_info,bool is_request);
    breakpoint_manager(breakpoint_filter& req, breakpoint_filter& rsp): req_filter(req),rsp_filter(rsp) {}
private:
    const breakpoint_filter& req_filter;
    const breakpoint_filter& rsp_filter;
};

