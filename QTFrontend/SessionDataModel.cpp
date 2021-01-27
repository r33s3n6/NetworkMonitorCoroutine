#include "SessionDataModel.h"

#include "../NetworkMonitorCoroutine/common_functions.h"
using namespace common;
#include <boost/lexical_cast.hpp>

SessionDataModel::SessionDataModel(QObject* parent, display_filter* _disp_fil)
    : QAbstractTableModel(parent),
    _data_vec()
{
    _data_vec.reserve(_default_capacity);

    connect(_disp_fil, &display_filter::session_created, this, &SessionDataModel::session_created);
    connect(_disp_fil, &display_filter::filter_updated, this, &SessionDataModel::filter_updated);
    connect(_disp_fil, &display_filter::session_created_breakpoint, this, &SessionDataModel::session_created_breakpoint);
    connect(_disp_fil, &display_filter::session_req_updated, this, &SessionDataModel::session_req_updated);
    connect(_disp_fil, &display_filter::session_rsp_updated, this, &SessionDataModel::session_rsp_updated);
    connect(_disp_fil, &display_filter::session_rsp_updated_breakpoint, this, &SessionDataModel::session_rsp_updated_breakpoint);
    connect(_disp_fil, &display_filter::session_error, this, &SessionDataModel::session_error);



    /*
    
    for (int i = 0; i < 3; i++) {
        _data_vec.emplace_back(&_test_data[i]);
    }
    */
    //connect()

}

int SessionDataModel::rowCount(const QModelIndex& /*parent*/) const
{
    return _data_vec.size();
}

int SessionDataModel::columnCount(const QModelIndex& /*parent*/) const
{
    return sizeof(_table_header_name)/sizeof(const char *);
}

QVariant SessionDataModel::data(const QModelIndex& index, int role) const
{
    if (role == Qt::DisplayRole) {
        session_info* temp = _data_vec[index.row()];
        if (temp == nullptr)
            return QString("data error");
        switch (index.column()) {
        case 0://#
            return QString::number(index.row());
        case 1://url
            return QString::fromStdString(temp->url);
        case 2://code
            return QString::fromStdString(temp->code);
        case 3://protocol
            return QString::fromStdString(temp->protocol);
        case 4://host
            return QString::fromStdString(temp->host);
        case 5://body
            return QString::number(temp->body_length);
        case 6://content-type
            return QString::fromStdString(temp->content_type);
        }

    }


    return QVariant();
}



QVariant SessionDataModel::headerData(int section, Qt::Orientation orientation, int role) const
{
   


    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        return QString(_table_header_name[section]);
    }
    return QVariant();
}

void SessionDataModel::filter_updated(string filter)
{
}

void SessionDataModel::session_created(shared_ptr<const string> req_data, int update_id)
{
    if (update_id >= _data_vec.capacity())
        _data_vec.reserve(_data_vec.capacity() * 2);

    session_info* info = new session_info;
    _data_vec[update_id] = info;

    shared_ptr<string> header= make_shared<string>();
    shared_ptr<string> body = make_shared<string>();
    split_request(req_data, header, body);


    shared_ptr<vector<string>> header_vec_ptr
        = string_split(*header, "\r\n");

    size_t url_start_pos = (*header_vec_ptr)[0].find(" ");
    size_t url_end_pos = (*header_vec_ptr)[0].find(" HTTP");
    if (url_start_pos != string::npos &&
        url_end_pos != string::npos) {
        info->url = (*header_vec_ptr)[0].substr(url_start_pos, url_end_pos - url_start_pos);
    }
    info->protocol = "TODO";
    //info->content_type = get_header_value(header_vec_ptr, "content-type");
    
    //info->body_length = boost::lexical_cast<int>(
    //    get_header_value(header_vec_ptr, "content-length"));
    info->host = get_header_value(header_vec_ptr, "host");
    info->raw_req_data = req_data;

   
}

void SessionDataModel::session_created_breakpoint(shared_ptr<string> req_data, int update_id)
{
    

}

void SessionDataModel::session_req_updated(shared_ptr<const string> req_data, int update_id)
{

}

void SessionDataModel::session_rsp_updated(shared_ptr<const string> rsp_data, int update_id)
{
    session_info* info = _data_vec[update_id];

    shared_ptr<string> header = make_shared<string>();
    shared_ptr<string> body = make_shared<string>();
    split_request(rsp_data, header, body);


    shared_ptr<vector<string>> header_vec_ptr
        = string_split(*header, "\r\n");

    size_t code_start_pos = (*header_vec_ptr)[0].find("HTTP/1.")+2;
    size_t code_end_pos = (*header_vec_ptr)[0].find(" ", code_start_pos);
    if (code_start_pos != string::npos &&
        code_end_pos != string::npos) {
        info->code = (*header_vec_ptr)[0].substr(code_start_pos, code_end_pos - code_start_pos);
    }

    info->content_type = get_header_value(header_vec_ptr, "content-type");

    info->body_length = boost::lexical_cast<int>(
        get_header_value(header_vec_ptr, "content-length"));
    //info->host = get_header_value(header_vec_ptr, "host");
    info->raw_rsp_data = rsp_data;
    //TODO ¸üÐÂ

}

void SessionDataModel::session_rsp_updated_breakpoint(shared_ptr<string> rsp_data, int update_id)
{
}

void SessionDataModel::session_error(shared_ptr<const string> err_msg, int update_id)
{
}
