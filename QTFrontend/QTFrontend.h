#pragma once

#include <QtWidgets/QMainWindow>
#include <QSortFilterProxyModel>

#include "ui_QTFrontend.h"

#include "SessionDataModel.h"


#include "../NetworkMonitorCoroutine/proxy_server.h"
using namespace proxy_tcp;

#include <map>
using namespace std;



#include "../NetworkMonitorCoroutine/config.h"


#include <boost/thread.hpp>





class QTFrontend : public QMainWindow
{
    Q_OBJECT

public:
    QTFrontend(QWidget *parent = Q_NULLPTR);


    ~QTFrontend() {
        _config->save_config();
        delete table_session_context_menu;
    }



private slots:
    void display_full_info(const QModelIndex& index, const QModelIndex& prev);
    void update_displayed_info(size_t update_id);

    void pass_session();
    void drop_session();
    void _debug_function();

    void _restart_backend_server();
    void _trust_root_ca();

signals:
    //void session_replayed(const QModelIndex& index,bool with_bp);


private:
    display_filter _display_filter;

    shared_ptr<proxy_server> _backend_server;

    shared_ptr<boost::thread> _backend_thread;

    Ui::QTFrontendClass ui;

    SessionDataModel _session_data;
    QSortFilterProxyModel _proxy_session_data;
    
    shared_ptr<session_info> _display_session_ptr;

    size_t _display_id=0;

    config* _config;

    QMenu* table_session_context_menu;

    bool last_breakpoint_req_checked=true;

    bool is_req_intercepted = true;

    
    void _setup_table();
    QModelIndex context_menu_index;
    void _replay_session(bool with_bp = false);
    void _replay_session_with_bp();

    void _display_table_context_menu(QPoint pos);

    void _display_full_info(size_t display_id);
    void _activate_breakpoint_box(bool active);
    void _activate_editor(bool active, bool is_req);

    

    void _toggle_breakpoint_config();

    void _set_enable_config();
    void _set_filter_vec(bool is_req);
    void _set_config();//write ui's data to _config
    void _display_config();

    

};
