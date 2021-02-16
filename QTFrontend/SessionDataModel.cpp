#include "SessionDataModel.h"

#include "../NetworkMonitorCoroutine/common_functions.h"
using namespace common;
#include <boost/lexical_cast.hpp>

#include <iostream>

SessionDataModel::SessionDataModel(QObject* parent, display_filter* _disp_fil)
    : QAbstractTableModel(parent),
    _data_vec()

{
    _data_vec.reserve(_default_capacity);
    //_data_vec.resize(_default_capacity / 2);
    connect(_disp_fil, &display_filter::session_created, this, &SessionDataModel::session_created, Qt::DirectConnection);
    connect(_disp_fil, &display_filter::filter_updated, this, &SessionDataModel::filter_updated, Qt::DirectConnection);
    connect(_disp_fil, &display_filter::session_created_breakpoint, this, &SessionDataModel::session_created_breakpoint, Qt::DirectConnection);
    connect(_disp_fil, &display_filter::session_req_updated, this, &SessionDataModel::session_req_updated, Qt::DirectConnection);
    connect(_disp_fil, &display_filter::session_rsp_updated, this, &SessionDataModel::session_rsp_updated, Qt::DirectConnection);
    connect(_disp_fil, &display_filter::session_rsp_updated_breakpoint, this, &SessionDataModel::session_rsp_updated_breakpoint, Qt::DirectConnection);
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
        session_info* temp = _data_vec[index.row()];
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

void SessionDataModel::filter_updated(string filter)
{
}

void SessionDataModel::session_created(shared_ptr<string> req_data, int update_id)
{


    std::cout << update_id<<":session_created\n";
    session_info* info;

   

    info = new session_info;
   
   

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

    info->host = get_header_value(header_vec_ptr, "host");
    info->raw_req_data = req_data;





    if (update_id < _data_vec.size()) {

        _data_vec[update_id] = info;//TODO:potention read/write conflict 
        emit dataChanged(createIndex(update_id, 0), createIndex(update_id, columnCount() - 1));
    }

    else {
        beginInsertRows(QModelIndex(), _data_vec.size(), update_id);
        _data_vec.resize(update_id + 1);
        _data_vec[update_id] = info;//TODO:potention read/write conflict
        endInsertRows();

    }


}

void SessionDataModel::session_created_breakpoint(shared_ptr<string> req_data, int update_id)
{
    

}

void SessionDataModel::session_req_updated(shared_ptr<string> req_data, int update_id)
{

}

void SessionDataModel::session_rsp_updated(shared_ptr<string> rsp_data, int update_id)
{
    std::cout << update_id << ":session_updated\n";
    if (update_id >= _data_vec.size()) {
        std::cout << "fck_rsp" << std::endl;
        return;
    }
   
    
    session_info* info = _data_vec[update_id];

    if (rsp_data->find("HTTP") != 0) {
        size_t body_start_pos = rsp_data->find("\r\n") + 2;

        info->raw_rsp_data->append(rsp_data->substr(body_start_pos,rsp_data->size()-2-body_start_pos));
        emit info_updated(update_id);
        return;
    }

    shared_ptr<string> header = make_shared<string>();
    shared_ptr<string> body = make_shared<string>();
    split_request(rsp_data, header, body);


    shared_ptr<vector<string>> header_vec_ptr
        = string_split(*header, "\r\n");
    // http/1.1 200 ok
    size_t code_start_pos = (*header_vec_ptr)[0].find("HTTP/")+9;
    size_t code_end_pos = (*header_vec_ptr)[0].find(" ", code_start_pos);


    if (code_start_pos != string::npos &&
        code_end_pos != string::npos) {
        info->code = (*header_vec_ptr)[0].substr(code_start_pos, code_end_pos - code_start_pos);
    }


    info->content_type = get_header_value(header_vec_ptr, "content-type");

    string l = get_header_value(header_vec_ptr, "content-length");
    if (l.size() == 0)
        info->body_length = 0;
    else
        info->body_length = boost::lexical_cast<int>(l);
 
    info->raw_rsp_data = rsp_data;
    emit dataChanged(createIndex(update_id, 0), createIndex(update_id, columnCount() - 1));


}

void SessionDataModel::session_rsp_updated_breakpoint(shared_ptr<string> rsp_data, int update_id)
{
}

void SessionDataModel::session_error(shared_ptr<string> err_msg, int update_id)
{
    //TODO emit signal to main dialog
    _data_vec[update_id]->raw_rsp_data=err_msg;
}
