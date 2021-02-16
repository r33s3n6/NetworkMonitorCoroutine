#pragma once

#include <QAbstractTableModel>

#include <memory>
#include <string>
#include <vector>
using namespace std;

constexpr int _default_capacity = 256;

#include "../NetworkMonitorCoroutine/display_filter.h"

#include <qtableview.h>


using namespace proxy_tcp;

struct session_info {
    string url;
    string code;
    string protocol;
    string host;
    int body_length;
    string content_type;
    shared_ptr<string> raw_req_data;
    shared_ptr<string> raw_rsp_data;
    session_info():body_length(0) {}
    session_info(string url,string code,string protocol,
        string host,int body_length,string content_type,
        shared_ptr<string> raw_req_data,
        shared_ptr<string> raw_rsp_data)
        :url(url),code(code),protocol(protocol),host(host),body_length(body_length),
        content_type(content_type),raw_req_data(raw_req_data),raw_rsp_data(raw_rsp_data)
    {};
};


static const char* _table_header_name[] = {
    "#", "URL","Code","Protocol","Host","Body","Content-Type"
};

static session_info _test_data[3]{
    {"/index.php","200","https","www.baidu.com",80,"text/html",make_shared<string>("test request data1"),make_shared<string>("test response data1")},
    {"/test.php","200","http","www.baidu.com",540,"text/html",make_shared<string>("test request data2"),make_shared<string>("test response data2")},
    {"/index.php","200","https","www.google.com",1050,"text/html",make_shared<string>("test request data3"),make_shared<string>("test response data3")}
};

class SessionDataModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    SessionDataModel(QObject* parent = nullptr,display_filter *_disp_fil = nullptr);
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;


    inline string get_content_type(size_t rank) { return _data_vec[rank]->content_type; }
    inline shared_ptr<string> get_raw_req_data(size_t rank) { return _data_vec[rank]->raw_req_data; }
    inline shared_ptr<string> get_raw_rsp_data(size_t rank){ return _data_vec[rank]->raw_rsp_data; }

private:
    vector<session_info*> _data_vec;
    QTableView* _table;

signals:
    void info_updated(size_t update_id);

private slots:
    void filter_updated(string filter);

    void session_created(shared_ptr<string> req_data, int update_id);
    void session_created_breakpoint(shared_ptr<string> req_data, int update_id);

    void session_req_updated(shared_ptr<string> req_data, int update_id);//本质上可以改，等待所有chunked data就绪

    void session_rsp_updated(shared_ptr<string> rsp_data, int update_id);
    void session_rsp_updated_breakpoint(shared_ptr<string> rsp_data, int update_id);

    void session_error(shared_ptr<string> err_msg, int update_id);


};

