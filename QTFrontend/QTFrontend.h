#pragma once

#include <QtWidgets/QMainWindow>
#include <QSortFilterProxyModel>

#include "ui_QTFrontend.h"

#include "SessionDataModel.h"

class QTFrontend : public QMainWindow
{
    Q_OBJECT

public:
    QTFrontend(QWidget *parent = Q_NULLPTR);


    void _set_text(const QModelIndex& index);
private:

    Ui::QTFrontendClass ui;

    SessionDataModel _session_data;
    QSortFilterProxyModel _proxy_session_data;
   
private:
    

};
