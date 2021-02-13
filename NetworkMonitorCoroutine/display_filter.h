#pragma once

#include <string>
#include <memory>
#include <functional>




#include <boost/asio.hpp>

#ifdef QT_CORE_LIB
#include <qobject.h>
#include <mutex>
//#include "../QTFrontend/SessionDataModel.h"
#endif

using namespace std;
namespace proxy_tcp{

	using boost::asio::ip::tcp;
	using boost::asio::awaitable;
	using boost::asio::co_spawn;
	using boost::asio::detached;
	using boost::asio::redirect_error;
	using boost::asio::use_awaitable;


#ifdef QT_CORE_LIB
class display_filter : public QObject
{
	Q_OBJECT
#else
class display_filter
{

#endif

public:
#ifndef QT_CORE_LIB
	void display(shared_ptr<string> req_data, shared_ptr<string> rsp_data);
	void _temp_display(shared_ptr<string> data);
#endif
	


	//初始显示
	int display(shared_ptr<string> req_data);
	
	//显示request的时候是不会有错的
	void update_display_req(int id, shared_ptr<string> req_data);

	void update_display_rsp(int id, shared_ptr<string> rsp_data);
	//只在更新response的时候顺便显示错误
	void update_display_error(int id, shared_ptr<string> rsp_data);

	//对于chunked data，只在第一次调用breakpoint
	awaitable<int> display_breakpoint_req(shared_ptr<string> req_data);//里面要显示出来
	awaitable<int> display_breakpoint_rsp(int update_id, shared_ptr<string> rsp_data);//里面要显示出来

	
#ifdef QT_CORE_LIB
private:
	mutex update_id_lock;
	size_t update_id = 0;

	//SessionDataModel* _session_data_model;
	inline size_t get_temp_id() {
		update_id_lock.lock();
		size_t temp_id = update_id;
		update_id++;
		update_id_lock.unlock();
		return temp_id;
	}

signals://signal
	void filter_updated(string filter);

	void session_created(shared_ptr<string> req_data, int update_id);
	void session_created_breakpoint(shared_ptr<string> req_data, int update_id);

	void session_req_updated(shared_ptr<string> req_data, int update_id);//本质上可以改，等待所有chunked data就绪

	void session_rsp_updated(shared_ptr<string> rsp_data, int update_id);
	void session_rsp_updated_breakpoint(shared_ptr<string> rsp_data,int update_id);

	void session_error(shared_ptr<string> err_msg, int update_id);

//private slots://slot
	//void session_passed(int update_id,bool is_req);//状态不知道应该用bitmap还是hashmap

#endif
};

}

 
