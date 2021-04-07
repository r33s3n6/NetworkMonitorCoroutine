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

    connect(_disp_fil, &display_filter::session_created, this, &SessionDataModel::session_created, Qt::DirectConnection);
    connect(_disp_fil, &display_filter::session_req_updated, this, &SessionDataModel::session_req_updated, Qt::DirectConnection);
    connect(_disp_fil, &display_filter::session_req_completed, this, &SessionDataModel::session_req_completed, Qt::DirectConnection);

    connect(_disp_fil, &display_filter::session_rsp_begin, this, &SessionDataModel::session_rsp_begin, Qt::DirectConnection);
    connect(_disp_fil, &display_filter::session_rsp_updated, this, &SessionDataModel::session_rsp_updated, Qt::DirectConnection);
    connect(_disp_fil, &display_filter::session_rsp_completed, this, &SessionDataModel::session_rsp_completed, Qt::DirectConnection);
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

    shared_ptr<session_info> temp = _data_vec[index.row()];
    if (role == Qt::DisplayRole) {
        
        if (temp == nullptr)
            return QString("N/A");
        switch (index.column()) {
        case 0://#
            return temp->id;
        case 1://url
            return QString::fromStdString(temp->url);
        case 2://code
            return QString::fromStdString(temp->code);
        case 3://protocol
            if (temp->protocol.size() == 0)
                return QString("unknown");
            return QString::fromStdString(temp->protocol);
        case 4://host
            return QString::fromStdString(temp->host);
        case 5://body
            return temp->body_length;
        case 6://content-type
            return QString::fromStdString(temp->content_type);
        }

    }
    else if (role == Qt::BackgroundColorRole)
    {
        if (temp) {
            if (temp->failed) {
                //return QColor(0xff, 0x7a, 0x37);
            }
            else if ((temp->send_behaviour == intercept)
                || (temp->receive_behaviour == intercept)) {
                //return QColor(0xff, 0x7a, 0x37);//橙色
                return QColor(0xff, 0xaa, 0xff);
                //return QColor(0xda, 0x4f, 0x49);

            }
                
            
        }
        
    }else if(role == Qt::ForegroundRole){
        if (temp) {
            if (temp->failed) {
                //return QColor(0xda, 0x4f, 0x49);//稍浅一点的红色
                return QColor(0xff, 0x00, 0x00);//大红
            }
            else if ((temp->send_behaviour == intercept)
                || (temp->receive_behaviour == intercept)) {
                return QColor(0xff, 0xff, 0xff);
                //return QColor(0xda, 0x4f, 0x49);

            }


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



void SessionDataModel::force_refresh(size_t display_id)
{
    emit dataChanged(createIndex(display_id, 0), createIndex(display_id, columnCount() - 1));
}




//slots


void SessionDataModel::session_created(shared_ptr<session_info> _session_info)
{
    
    if (_session_info->new_data) {
        _session_info->req_data_for_display = make_shared<string>(*(_session_info->new_data));

        shared_ptr<string> header = make_shared<string>();
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


        _session_info->host = get_header_value(header_vec_ptr, "host");
    } 
    else {
        _session_info->req_data_for_display = make_shared<string>("Data Broken"); 
    }
        

    id_locker.lock();
    _session_info->id = id;
    id++;

    beginInsertRows(QModelIndex(), _data_vec.size(), _data_vec.size());
    data_locker.lock();
    _data_vec.emplace_back(_session_info);
    data_locker.unlock();
    endInsertRows();

    id_locker.unlock();

    if (_session_info->send_behaviour == intercept) {
        emit session_intercepted(_session_info,true);//request

    }


    std::cout << _session_info->id << ":session_created\n";
    stringstream ss;
    ss << _session_info->id << ":session_created";
    string msg;
    ss >> msg;
    emit status_updated(msg);


}

void SessionDataModel::session_req_updated(shared_ptr<session_info> _session_info)//chunked data
{
    if (_session_info->id == -1)
        return;

    std::cout << _session_info->id << ":session_request_updated\n";
    stringstream ss;
    ss << _session_info->id << ":session_request_updated";
    string msg;
    ss >> msg;
    emit status_updated(msg);

    size_t body_start_pos = _session_info->new_data->find("\r\n") + 2;
    
    _session_info->req_data_for_display->append(
        _session_info->new_data->substr(
            body_start_pos, _session_info->new_data->size() - 2 - body_start_pos));
    


}

void SessionDataModel::session_req_completed(shared_ptr<session_info> _session_info)
{
    std::cout << _session_info->id << ":session_request_completed\n";
    stringstream ss;
    ss << _session_info->id << ":session_request_completed";
    string msg;
    ss >> msg;
    emit status_updated(msg);
    _session_info->req_completed = true;
    emit info_updated(_session_info->id);
}




void SessionDataModel::session_rsp_begin(shared_ptr<session_info> _session_info)
{
    if (_session_info->id == -1)
        return;

    std::cout << _session_info->id << ":session_response_begin\n";
    stringstream ss;
    ss << _session_info->id << ":session_response_begin";
    string msg;
    ss >> msg;
    emit status_updated(msg);

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
    emit info_updated(_session_info->id);

    if (_session_info->receive_behaviour == intercept) {
        emit session_intercepted(_session_info,false);//response
    }

}

void SessionDataModel::session_rsp_updated(shared_ptr<session_info> _session_info)
{
    if (_session_info->id == -1)
        return;

    std::cout << _session_info->id << ":session_response_updated\n";
    stringstream ss;
    ss << _session_info->id << ":session_response_updated";
    string msg;
    ss >> msg;
    emit status_updated(msg);


    size_t body_start_pos = _session_info->new_data->find("\r\n") + 2;

    string&& body = _session_info->new_data->substr(
        body_start_pos, _session_info->new_data->size() - 2 - body_start_pos);
    _session_info->body_length += body.size();
    _session_info->rsp_data_for_display->append(body);

    





}

void SessionDataModel::session_rsp_completed(shared_ptr<session_info> _session_info)
{
    std::cout << _session_info->id << ":session_response_completed\n";
    stringstream ss;
    ss << _session_info->id << ":session_response_completed";
    string msg;
    ss >> msg;
    emit status_updated(msg);
    _session_info->rsp_completed = true;
    emit info_updated(_session_info->id);
}



void SessionDataModel::session_error(shared_ptr<session_info> _session_info)
{

    _session_info->rsp_data_for_display = make_shared<string>(*(_session_info->new_data));
    emit info_updated(_session_info->id);
}

bool SessionDataModel::removeRows(const vector<int>& indexes)
{
    data_locker.lock();
    for (int index : indexes) {//index为降序，移除将不对下标产生影响
        beginRemoveRows(QModelIndex(), index, index);
        _data_vec.erase(_data_vec.begin() + index);
        endRemoveRows();
    }
    data_locker.unlock();
    return false;
}

void SessionDataModel::delete_session(const string& host) {
    vector<int> to_be_removed;//desc
    
    for (int i = _data_vec.size()-1; i >-1; i--) {
        
        if (_data_vec[i]->host == host) {
            to_be_removed.emplace_back(i);
        }
    }

    removeRows(to_be_removed);
   
}

/*
void SessionDataModel::session_replayed(const QModelIndex& index,bool with_bp)
{
   //deprecated
    //_session_info->rsp_data_for_display = make_shared<string>(*(_session_info->new_data));


    //emit info_updated(_session_info->id);
}*/

