#include "breakpoint_manager.h"





bool breakpoint_manager::check(shared_ptr<const session_info> _session_info, bool is_request)
{
    //TODO
    if (_session_info->raw_req_data->find("Host: www.baidu.com") != string::npos) {
        return true;
    }
       



    return false;
}
