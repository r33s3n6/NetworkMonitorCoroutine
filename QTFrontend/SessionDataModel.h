#pragma once

#include <QAbstractTableModel>

#include <memory>
#include <string>
#include <vector>
using namespace std;

constexpr int _default_capacity = 256;

//#include "../NetworkMonitorCoroutine/display_filter.h"

#include <qtableview.h>
#include <mutex>

#include "../NetworkMonitorCoroutine/config.h"

//using namespace proxy_tcp;
namespace proxy_tcp {
    class display_filter;
    class http_proxy_handler;
}

using namespace proxy_tcp;

typedef enum {
    undefined,
    pass,
    intercept,
    pass_after_intercept,
    drop
    
}session_behaviour;

struct session_info {//应该把req/rsp的数据单独抽象出一个struct 更加方便管理
    size_t id;
    shared_ptr<http_proxy_handler> proxy_handler_ptr;
    session_behaviour send_behaviour;
    session_behaviour receive_behaviour;
    string url;
    string code;
    string protocol;
    string host;
    int body_length;
    string content_type;
    bool req_completed=false;
    bool rsp_completed = false;
    shared_ptr<string> raw_req_data;
    shared_ptr<string> raw_rsp_data;
    shared_ptr<string> req_data_for_display;
    shared_ptr<string> rsp_data_for_display;
    shared_ptr<const string> new_data;
    bool edited = false;//req rsp 共用
    string temp_req_data;
    string temp_rsp_data;//切换时暂时保存
    session_info() :body_length(0), id(0) { 
        proxy_handler_ptr.reset(); 
        send_behaviour = undefined;
        receive_behaviour = undefined;
    }

    
    /*
    session_info(string url, string code, string protocol,
        string host, int body_length, string content_type,
        shared_ptr<string> raw_req_data,
        shared_ptr<string> raw_rsp_data)
        :id(0),url(url), code(code), protocol(protocol), host(host), body_length(body_length),
        content_type(content_type), raw_req_data(raw_req_data), raw_rsp_data(raw_rsp_data)
    {};*/
};




/*
static session_info _test_data[3]{
    {"/index.php","200","https","www.baidu.com",80,"text/html",make_shared<string>("test request data1"),make_shared<string>("test response data1")},
    {"/test.php","200","http","www.baidu.com",540,"text/html",make_shared<string>("test request data2"),make_shared<string>("test response data2")},
    {"/index.php","200","https","www.google.com",1050,"text/html",make_shared<string>("test request data3"),make_shared<string>("test response data3")}
};*/




class SessionDataModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    SessionDataModel(QObject* parent = nullptr, display_filter *_disp_fil = nullptr);
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    inline shared_ptr<session_info> get_session_info_ptr(size_t r) { return _data_vec[r]; }

    inline string get_content_type(size_t rank) { return _data_vec[rank]->content_type; }
    inline shared_ptr<string> get_raw_req_data(size_t rank) { return _data_vec[rank]->raw_req_data; }
    inline shared_ptr<string> get_raw_rsp_data(size_t rank){ return _data_vec[rank]->raw_rsp_data; }

    void force_refresh(size_t display_id);

private:
    vector<shared_ptr<session_info>> _data_vec;
    QTableView* _table;
    size_t id = 0;
    mutex id_locker;

signals:
    void info_updated(size_t update_id);
    void session_intercepted(shared_ptr<session_info> _session_info,bool is_req);

private slots:
    void session_created(shared_ptr<session_info> _session_info);

    void session_req_updated(shared_ptr<session_info> _session_info);
    void session_req_completed(shared_ptr<session_info> _session_info);

    void session_rsp_begin(shared_ptr<session_info> _session_info);
    void session_rsp_updated(shared_ptr<session_info> _session_info);
    void session_rsp_completed(shared_ptr<session_info> _session_info);

    void session_error(shared_ptr<session_info> _session_info);

public slots:
    //void session_replayed(const QModelIndex& index, bool with_bp);

    //void pass_session(shared_ptr<session_info> _session_info);
    //void drop_session(shared_ptr<session_info> _session_info);

};

