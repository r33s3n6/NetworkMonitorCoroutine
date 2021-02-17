#pragma once

#include <QtWidgets/QMainWindow>
#include <QSortFilterProxyModel>

#include "ui_QTFrontend.h"

#include "SessionDataModel.h"

using namespace proxy_tcp;

class QTFrontend : public QMainWindow
{
    Q_OBJECT

public:
    QTFrontend(QWidget *parent = Q_NULLPTR, display_filter* _disp = nullptr);






private slots:
    void display_full_info(const QModelIndex& index, const QModelIndex& prev);
    void update_displayed_info(size_t update_id);

    void pass_session();
    void drop_session();
    void _debug_function();

private:
    Ui::QTFrontendClass ui;

    SessionDataModel _session_data;
    QSortFilterProxyModel _proxy_session_data;
    
    shared_ptr<session_info> _display_session_ptr;

    size_t _display_id=0;

    bool is_req_intercepted = true;

    
    void _display_full_info(size_t display_id);
    void _activate_breakpoint_box(bool active);
    void _activate_editor(bool active, bool is_req);

    

};
