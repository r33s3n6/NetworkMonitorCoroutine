#include "QTFrontend.h"

static int column_width[] = {
    20,100,35,45,200,40,70
};

QTFrontend::QTFrontend(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);

    //table settings start
    _proxy_session_data.setSourceModel(&_session_data);

    ui.table_session->setModel(&_proxy_session_data);
    //ui.table_session->setSortingEnabled(true);
    ui.table_session->sortByColumn(0, Qt::AscendingOrder);

    //ui.table_session->setSelectionBehavior(QAbstractItemView::SelectRows);

    //ui.table_session->verticalHeader()->setHidden(true);
    //ui.table_session->verticalHeader()->setDefaultSectionSize(15);

    //ui.table_session->horizontalHeader()->setHighlightSections(false);

    for (int i = 0; i < sizeof(column_width) / sizeof(int); i++) {
        ui.table_session->setColumnWidth(i, column_width[i]); //TODO : config
    }

    //connect(ui.table_session, SIGNAL(activated(const QModelIndex&)), this, SLOT(_set_text(const QModelIndex&)));
    //connect(ui.table_session, SIGNAL(clicked(const QModelIndex&)), this, SLOT(_set_text(const QModelIndex&)));

    connect(ui.table_session, &QTableView::clicked, this, &QTFrontend::_set_text);
    ui.table_session->show();
    //table settings end

}

void QTFrontend::_set_text(const QModelIndex& index)
{
    //size_t rank = index.row();

    //绕远路了，但先这样吧
    size_t rank = _proxy_session_data.data(_proxy_session_data.index(index.row(), 0)).toInt();// 获取实际位置

    ui.plaintext_req_text->setPlainText(
        QString::fromStdString(*_session_data.get_raw_req_data(rank)));
    ui.plaintext_rsp_text->setPlainText(
        QString::fromStdString(*_session_data.get_raw_rsp_data(rank)));
}
