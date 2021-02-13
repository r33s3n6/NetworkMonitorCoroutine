#pragma once

#include <QtWidgets/QMainWindow>
#include <QSortFilterProxyModel>

#include "ui_QTFrontend.h"

#include "SessionDataModel.h"

class QTFrontend : public QMainWindow
{
    Q_OBJECT

public:
    QTFrontend(QWidget *parent = Q_NULLPTR, display_filter* _disp = nullptr);


    void _set_text(const QModelIndex& index, const QModelIndex& prev);
private:

    Ui::QTFrontendClass ui;

    SessionDataModel _session_data;
    QSortFilterProxyModel _proxy_session_data;
   

    

    

};
