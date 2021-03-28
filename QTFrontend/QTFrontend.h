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

#include "LicenseWindow.h"



class QTFrontend : public QMainWindow
{
    Q_OBJECT

public:
    QTFrontend(QWidget *parent = Q_NULLPTR,bool debug=false);


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
    QModelIndex _display_index;
    size_t _display_row;

    config* _config;

    QMenu* table_session_context_menu;
    QMenu* table_session_bp_menu;

    bool last_breakpoint_req_checked=true;

    bool is_req_intercepted = true;
    
    LicenseWindow license_window;
    
    void _setup_table();
    shared_ptr<session_info> _context_menu_session;
    //QModelIndex context_menu_index;
    //QString context_menu_host;

    void _add_bp(bool is_req);
    void _delete_bp(bool is_req);

    void _add_req_bp() { 
        if (ui.action_req_bp->isChecked()) {
            //if (ui.action_rsp_bp->isChecked()) {
            //    ui.action_both_bp->setChecked(true);
            //}

            _add_bp(true);
        }
        else {
            //ui.action_both_bp->setChecked(false);
            _delete_bp(true);
        }
       
    }
    void _add_rsp_bp() {
        if (ui.action_rsp_bp->isChecked()) {
            //if (ui.action_req_bp->isChecked()) {
            //    ui.action_both_bp->setChecked(true);
            //}
            _add_bp(false);
        }
        else {
            //ui.action_both_bp->setChecked(false);
            _delete_bp(false);
        }
    }
    void _add_both_bp(){
        ui.action_req_bp->setChecked(ui.action_both_bp->isChecked());
        ui.action_rsp_bp->setChecked(ui.action_both_bp->isChecked());
        _add_req_bp();
        _add_rsp_bp();
    }
    
    void _save_data_to_hexeditor(shared_ptr<session_info> _session_info_ptr);
    string _raw_chunk_to_text(const string& data);

    void _replay_session(bool with_bp = false);
    void _replay_session_with_bp();

    void _display_table_context_menu(QPoint pos);

    void _display_full_info(size_t display_row);
    void _activate_breakpoint_box(bool active);
    void _activate_editor(bool active, bool is_req);

    

    void _toggle_breakpoint_config();

    void _set_enable_config();
    //void _set_filter_vec(bool is_req);
    void _set_filter_vec(vector<http_header_filter>& filter_vec, const string& raw_filter, const string& raw_url_filter = "", bool reverse_behaviour_url = false);
    void _set_breakpoint_filter(bool is_req);
    void _set_display_filter();
    void _set_config();//write ui's data to _config
    void _display_config();

    void _row_removed(QModelIndex ignore, int first, int last) {
        if (first <= _display_row && last >= _display_row) {
            _display_row = -1;
            _display_id = -1;
            _display_full_info(_display_row);
        }

    }
    void closeEvent(QCloseEvent *event) {

        _set_config();
    }
};
