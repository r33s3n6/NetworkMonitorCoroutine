#pragma once
#include <string>

using namespace std;

#include "../QTFrontend/SessionDataModel.h"

class breakpoint_manager
{
public:
	bool check(shared_ptr<const session_info> _session_info,bool is_request);

};

