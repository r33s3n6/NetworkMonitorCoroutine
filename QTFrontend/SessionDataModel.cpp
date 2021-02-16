#include "SessionDataModel.h"

#include "../NetworkMonitorCoroutine/common_functions.h"
using namespace common;
#include <boost/lexical_cast.hpp>

#include <iostream>

#include "../NetworkMonitorCoroutine/display_filter.h"

using namespace proxy_tcp;

SessionDataModel::SessionDataModel(QObject* parent, display_filter* _disp_fil)
    : QAbstractTableModel(parent),
    _data_vec()

{
    _data_vec.reserve(_default_capacity);
    //_data_vec.resize(_default_capacity / 2);
    connect(_disp_fil, &display_filter::session_created, this, &SessionDataModel::session_created, Qt::DirectConnection);
    connect(_disp_fil, &display_filter::session_req_updated, this, &SessionDataModel::session_req_updated, Qt::DirectConnection);

    connect(_disp_fil, &display_filter::session_rsp_begin, this, &SessionDataModel::session_rsp_begin, Qt::DirectConnection);
    connect(_disp_fil, &display_filter::session_rsp_updated, this, &SessionDataModel::session_rsp_updated, Qt::DirectConnection);

    connect(_disp_fil, &display_filter::session_error, this, &SessionDataModel::session_error, Qt::DirectConnection);



 

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
        shared_ptr<session_info> temp = _data_vec[index.row()];
        if (temp == nullptr)
            return QString("N/A");
        switch (index.column()) {
        case 0://#
            return index.row();
        case 1://url
            return QString::fromStdString(temp->url);
        case 2://code
            return QString::fromStdString(temp->code);
        case 3://protocol
            if (temp->protocol.size() == 0)
                return QString("todo");
            return QString::fromStdString(temp->protocol);
        case 4://host
            return QString::fromStdString(temp->host);
        case 5://body
            return temp->body_length;
        case 6://content-type
            return QString::fromStdString(temp->content_type);
        }

    }


    return QVariant();
}



QVariant SessionDataModel::headerData(int section, Qt::Orientation orientation, int role) const
{
   //insertRow()


    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        return QString(_table_header_name[section]);
    }
    return QVariant();
}




//slots


void SessionDataModel::session_created(shared_ptr<session_info> _session_info)
{
    
    _session_info->id = id;
    id++;//TODO:potention read/write conflict 
    std::cout << _session_info->id <<":session_created\n";

    _session_info->req_data_for_display = make_shared<string>(*(_session_info->new_data));


    shared_ptr<string> header= make_shared<string>();
    shared_ptr<string> body = make_shared<string>();
    split_request(_session_info->req_data_for_display, header, body);


    shared_ptr<vector<string>> header_vec_ptr
        = string_split(*header, "\r\n");

    size_t url_start_pos = (*header_vec_ptr)[0].find(" ");
    size_t url_end_pos = (*header_vec_ptr)[0].find(" HTTP");
    if (url_start_pos != string::npos &&
        url_end_pos != string::npos) {
        _session_info->url = (*header_vec_ptr)[0].substr(url_start_pos, url_end_pos - url_start_pos);
    }
    //_session_info->protocol = "TODO";

    _session_info->host = get_header_value(header_vec_ptr, "host");
    


    
    //emit dataChanged(createIndex(_session_info->id, 0), createIndex(_session_info->id, columnCount() - 1));
    beginInsertRows(QModelIndex(), _session_info->id, _session_info->id);
    _data_vec.emplace_back(_session_info);
    endInsertRows();



}

void SessionDataModel::session_req_updated(shared_ptr<session_info> _session_info)//chunked data
{
    if (_session_info->id == -1)
        return;

    std::cout << _session_info->id << ":session_request_updated\n";


    size_t body_start_pos = _session_info->new_data->find("\r\n") + 2;
    
    _session_info->req_data_for_display->append(
        _session_info->new_data->substr(
            body_start_pos, _session_info->new_data->size() - 2 - body_start_pos));
    emit info_updated(_session_info->id);


}

void SessionDataModel::session_req_completed(shared_ptr<session_info> _session_info)
{
}




void SessionDataModel::session_rsp_begin(shared_ptr<session_info> _session_info)
{
    if (_session_info->id == -1)
        return;

    std::cout << _session_info->id << ":session_response_begin\n";

    _session_info->rsp_data_for_display = make_shared<string>(*(_session_info->new_data));

    shared_ptr<string> header = make_shared<string>();
    shared_ptr<string> body = make_shared<string>();
    split_request(_session_info->rsp_data_for_display, header, body);


    shared_ptr<vector<string>> header_vec_ptr
        = string_split(*header, "\r\n");
    // http/1.1 200 ok
    size_t code_start_pos = (*header_vec_ptr)[0].find("HTTP/") + 9;
    size_t code_end_pos = (*header_vec_ptr)[0].find(" ", code_start_pos);


    if (code_start_pos != string::npos &&
        code_end_pos != string::npos) {
        _session_info->code = (*header_vec_ptr)[0].substr(code_start_pos, code_end_pos - code_start_pos);
    }


    _session_info->content_type = get_header_value(header_vec_ptr, "content-type");

    string l = get_header_value(header_vec_ptr, "content-length");
    if (l.size() == 0)
        _session_info->body_length = 0;
    else
        _session_info->body_length = boost::lexical_cast<int>(l);


    emit dataChanged(createIndex(_session_info->id, 0), createIndex(_session_info->id, columnCount() - 1));
}

void SessionDataModel::session_rsp_updated(shared_ptr<session_info> _session_info)
{
    if (_session_info->id == -1)
        return;

    std::cout << _session_info->id << ":session_response_updated\n";



    size_t body_start_pos = _session_info->new_data->find("\r\n") + 2;
    
    _session_info->rsp_data_for_display->append(
        _session_info->new_data->substr(
            body_start_pos, _session_info->new_data->size()-2-body_start_pos));

    emit info_updated(_session_info->id);





}

void SessionDataModel::session_rsp_completed(shared_ptr<session_info> _session_info)
{
}



void SessionDataModel::session_error(shared_ptr<session_info> _session_info)
{
    //TODO emit signal to main dialog
    _session_info->rsp_data_for_display = make_shared<string>(*(_session_info->new_data));
    emit info_updated(_session_info->id);
}
