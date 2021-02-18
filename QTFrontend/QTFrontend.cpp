#include "QTFrontend.h"
#include "qimage.h"


static int column_width[] = {
    20,200,35,45,200,40,70
};

QTFrontend::QTFrontend(QWidget *parent,display_filter* _disp)
    : QMainWindow(parent),
    _session_data(nullptr,_disp)
{
    ui.setupUi(this);

    //table settings start
 
    _proxy_session_data.setSourceModel(&_session_data);
    
    ui.table_session->setModel(&_proxy_session_data);
    //ui.table_session->setSortingEnabled(true);
    ui.table_session->sortByColumn(0, Qt::AscendingOrder);
    //ui.table_session->setAlternatingRowColors(true);
    //ui.table_session->setSelectionBehavior(QAbstractItemView::SelectRows);

    //ui.table_session->verticalHeader()->setHidden(true);
    //ui.table_session->verticalHeader()->setDefaultSectionSize(15);

    //ui.table_session->horizontalHeader()->setHighlightSections(false);

    for (int i = 0; i < sizeof(column_width) / sizeof(int); i++) {
        ui.table_session->setColumnWidth(i, column_width[i]); //TODO : config
    }
    ui.table_session->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);

    connect(ui.table_session->selectionModel(), &QItemSelectionModel::currentRowChanged, this, &QTFrontend::display_full_info);
    connect(&_session_data, &SessionDataModel::info_updated, this, &QTFrontend::update_displayed_info);
    connect(ui.actionShow_Hide_breakpoint_button, &QAction::triggered, this, &QTFrontend::_debug_function);

    connect(ui.pass_session_button, &QPushButton::clicked, this, &QTFrontend::pass_session);
    connect(ui.drop_session_button, &QPushButton::clicked, this, &QTFrontend::drop_session);

    connect(ui.actionShow_Hide_breakpoint_button, &QAction::triggered, this, &QTFrontend::_debug_function);
    

    ui.table_session->show();
    //table settings end

    ui.scrollArea->setVisible(true);

    _activate_breakpoint_box(false);
    
}

void QTFrontend::display_full_info(const QModelIndex& index, const QModelIndex& prev) {
    
    _display_id = _proxy_session_data.data(_proxy_session_data.index(index.row(), 0)).toInt();// 获取实际位置

    _display_full_info(_display_id);
}

void QTFrontend::update_displayed_info(size_t update_id)
{
    if (_display_id == update_id) {
        _display_full_info(_display_id);
    }
}


void QTFrontend::pass_session() {
    auto _session_info_ptr = _session_data.get_session_info_ptr(_display_id);
    _activate_breakpoint_box(false);
    _activate_editor(false, is_req_intercepted);//_disable_editor of request/response

    
    if (is_req_intercepted) {
        _session_info_ptr->raw_req_data = make_shared<string>(
            ui.hexEdit_req->data().constData(), ui.hexEdit_req->data().length());
        _session_info_ptr->send_behaviour = pass_after_intercept;
    }
    else {

        _session_info_ptr->raw_rsp_data = make_shared<string>(
            ui.hexEdit_rsp->data().constData(), ui.hexEdit_rsp->data().length());
        _session_info_ptr->receive_behaviour = pass_after_intercept;
    }

    _session_data.force_refresh(_display_id);

}


void QTFrontend::drop_session() {
    auto _session_info_ptr = _session_data.get_session_info_ptr(_display_id);
    _activate_breakpoint_box(false);
    _activate_editor(false, is_req_intercepted);//_disable_editor of request/response

    if (is_req_intercepted) {
        _session_info_ptr->send_behaviour = drop;
    }
    else {
        _session_info_ptr->receive_behaviour = drop;
    }

    _session_data.force_refresh(_display_id);

}


void QTFrontend::_debug_function() {
    
    
    //_activate_breakpoint_box(false);
}


void QTFrontend::_activate_breakpoint_box(bool active){

    ui.pass_session_button->setDisabled(!active);
    ui.drop_session_button->setDisabled(!active);

    if (active) {
        ui.breakpoint_button_box->show();
        //ui.breakpoint_button_box->setMaximumHeight(33);
    }
    else {
        ui.breakpoint_button_box->hide();
        //ui.breakpoint_button_box->setFixedHeight(0);
    }
    
}



void QTFrontend::_activate_editor(bool active,bool is_req) {
    if (is_req) {
        ui.hexEdit_req->setReadOnly(!active);
        ui.plaintext_req_text->setReadOnly(!active);
    }
    else {
        ui.hexEdit_rsp->setReadOnly(!active);
        ui.plaintext_rsp_text->setReadOnly(!active);
    }
    

}

//TODO: 增加保存文件功能
void QTFrontend::_display_full_info(size_t display_id)
{

    

    auto _session_info_ptr = _session_data.get_session_info_ptr(display_id);

    if (_session_info_ptr->send_behaviour == intercept) {
        ui.label_intercept->setText("Request has been intercepted");
        _activate_breakpoint_box(true);
        _activate_editor(true, true);//_activate_editor of request
        is_req_intercepted = true;
        
    }
    else if (_session_info_ptr->receive_behaviour == intercept) {
        ui.label_intercept->setText("Response has been intercepted");
        _activate_breakpoint_box(true);
        _activate_editor(true, false);//_activate_editor of response
        is_req_intercepted = false;
    }
    else {
        _activate_breakpoint_box(false);
    }

    


    if (_session_info_ptr->req_data_for_display) {
        ui.plaintext_req_text->setPlainText(
            QString::fromStdString(*(_session_info_ptr->req_data_for_display)));
    }  
    else {
        ui.plaintext_req_text->setPlainText(QString());
    }

    if (_session_info_ptr->raw_req_data) {
        ui.hexEdit_req->setData(
            QByteArray(_session_info_ptr->raw_req_data->c_str(),
                _session_info_ptr->raw_req_data->size()));
    }
    else {
        ui.hexEdit_req->setData(QByteArray());
    }

    if (_session_info_ptr->raw_rsp_data) {
        ui.hexEdit_rsp->setData(
            QByteArray(_session_info_ptr->raw_rsp_data->c_str(),
                _session_info_ptr->raw_rsp_data->size()));
    }
    else {
        ui.hexEdit_rsp->setData(QByteArray());
    }

    if (!(_session_info_ptr->rsp_data_for_display)) {
        ui.plaintext_rsp_text->setPlainText(QString());
        return;
    }

    ui.plaintext_rsp_text->setPlainText(
        QString::fromStdString(*(_session_info_ptr->rsp_data_for_display)));


    try
    {
        if (_session_data.get_content_type(display_id).find("image") == 0) {//image
            size_t header_end_pos = _session_info_ptr->rsp_data_for_display->find("\r\n\r\n");

            if (header_end_pos != string::npos) {
                int length = _session_info_ptr->rsp_data_for_display->size() - header_end_pos - 4;
                const uchar* image_bytes = (const uchar*)(_session_info_ptr->rsp_data_for_display->c_str() + header_end_pos + 4);
                QImage img;
                if (!img.loadFromData(image_bytes, length))
                    throw -1;

                ui.label_image->setPixmap(QPixmap::fromImage(img));//TODO: gif/webm/... support
                ui.scrollArea->setWidgetResizable(false);
                ui.label_image->adjustSize();
                ui.scrollAreaWidgetContents->adjustSize();

                
            }
            else {
                throw - 1;
            }

        }else {
            throw -1;

        }

    }
    catch (int error_code)
    {
        if (error_code == -1) {
            ui.scrollArea->setWidgetResizable(true);
            ui.label_image->setText("Cannot resolve it as image");
        }

    }
    

}
