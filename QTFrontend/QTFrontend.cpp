#include "QTFrontend.h"
#include "qimage.h"




QTFrontend::QTFrontend(QWidget *parent, proxy_server* _backend_server)
    : QMainWindow(parent),
    _session_data(nullptr, _backend_server->get_display_filter()),
    _config(_backend_server->get_config_ref())
{
    ui.setupUi(this);

    _load_config_from_file("config.dat");
    _display_config();
    //_set_config();//可视化config

    //table settings start
 
    _proxy_session_data.setSourceModel(&_session_data);
    
    ui.table_session->setModel(&_proxy_session_data);


    ui.table_session->sortByColumn(0, Qt::AscendingOrder);



    for (int i = 0; i < sizeof(_config.column_width) / sizeof(int); i++) {
        ui.table_session->setColumnWidth(i, _config.column_width[i]);
    }
    ui.table_session->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);

    connect(ui.table_session->selectionModel(), &QItemSelectionModel::currentRowChanged, this, &QTFrontend::display_full_info);
    connect(&_session_data, &SessionDataModel::info_updated, this, &QTFrontend::update_displayed_info);
    connect(ui.actionShow_Hide_breakpoint_button, &QAction::triggered, this, &QTFrontend::_debug_function);

    connect(ui.pass_session_button, &QPushButton::clicked, this, &QTFrontend::pass_session);
    connect(ui.drop_session_button, &QPushButton::clicked, this, &QTFrontend::drop_session);

    connect(ui.actionShow_Hide_breakpoint_button, &QAction::triggered, this, &QTFrontend::_debug_function);
    
    connect(ui.radioButton_req_settings, &QRadioButton::toggled, this, &QTFrontend::_toggle_breakpoint_config);
    connect(ui.radioButton_rsp_settings, &QRadioButton::toggled, this, &QTFrontend::_toggle_breakpoint_config);

    connect(ui.pushButton_set_config, &QPushButton::clicked, this, &QTFrontend::_set_config);

    connect(ui.checkBox_enable_req_breakpoint, &QCheckBox::toggled,this, &QTFrontend::_set_enable_config);
    connect(ui.checkBox_enable_rsp_breakpoint, &QCheckBox::toggled,this,&QTFrontend::_set_enable_config);
    //connect(ui.lineEdit_breakpoint_host, &QLineEdit::editingFinished, this, &QTFrontend::_set_config);
    //connect(ui.plainTextEdit_breakpoint_custom, &QPlainTextEdit::finish, this, &QTFrontend::_set_config);
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


//configuration

void QTFrontend::_load_config_from_file(string path)
{
    _config.req_filter.raw_custom_header_filter = "";
    _config.req_filter.raw_host_filter = "*.baidu.com";
    _set_filter_map(true);

    _config.rsp_filter.raw_custom_header_filter = "";
    _config.rsp_filter.raw_host_filter = "*.sohu.com";
    _set_filter_map(false);
}

void QTFrontend::_set_enable_config() {
    _config.req_filter.enable_breakpoint =
        ui.checkBox_enable_req_breakpoint->isChecked();

    _config.rsp_filter.enable_breakpoint =
        ui.checkBox_enable_rsp_breakpoint->isChecked();
}
void QTFrontend::_set_config()
{
    _set_enable_config();
    
    bool is_req = ui.radioButton_req_settings->isChecked();
    auto& filter = is_req ? _config.req_filter : _config.rsp_filter;

    bool update_filter_map = false;
    string&& temp_custom = ui.plainTextEdit_breakpoint_custom->toPlainText().toStdString();
    string && temp_host = ui.lineEdit_breakpoint_host->text().toStdString();
    if (filter.raw_custom_header_filter != temp_custom) {
        filter.raw_custom_header_filter = temp_custom;
        update_filter_map = true;
    }
    if (filter.raw_host_filter != temp_host) {
        filter.raw_host_filter = temp_host;
        update_filter_map = true;
    }
    if(update_filter_map)
        _set_filter_map(is_req);

}

void QTFrontend::_set_filter_map(bool is_req) {

    breakpoint_filter& filter = is_req ? _config.req_filter : _config.rsp_filter;

    shared_ptr<vector<string>> header_vec_ptr
        = string_split(filter.raw_custom_header_filter, "\r\n");

    if(filter.raw_host_filter.size()>0)
        header_vec_ptr->emplace_back("host:"+filter.raw_host_filter);
    
    filter.header_filter=make_shared<map<string,string>>();


    for (auto header : *header_vec_ptr) {
        size_t pos = header.find(":");
        if (pos == string::npos || (pos + 1) == header.size())
            continue;
        string temp = header.substr(0, pos);
        transform(temp.begin(), temp.end(), temp.begin(), ::tolower);

        (*filter.header_filter)[string_trim(temp)] = 
            string_trim(header.substr(pos + 1, header.size() - pos - 1));

    }
}

void QTFrontend::_toggle_breakpoint_config()
{
    
    if (ui.radioButton_req_settings->isChecked() ^ last_breakpoint_req_checked) {//不同
        

        if (ui.radioButton_req_settings->isChecked()) {
            _config.rsp_filter.raw_custom_header_filter = ui.plainTextEdit_breakpoint_custom->toPlainText().toStdString();
            _config.rsp_filter.raw_host_filter = ui.lineEdit_breakpoint_host->text().toStdString();
        }
        else {
            _config.req_filter.raw_custom_header_filter = ui.plainTextEdit_breakpoint_custom->toPlainText().toStdString();
            _config.req_filter.raw_host_filter = ui.lineEdit_breakpoint_host->text().toStdString();

        }
        _display_config();


        last_breakpoint_req_checked = ui.radioButton_req_settings->isChecked();
    }
    //相同不作处理
       
        
    
    

    
    

}

void QTFrontend::_display_config()
{
    ui.checkBox_enable_req_breakpoint->setChecked(_config.req_filter.enable_breakpoint);
    ui.checkBox_enable_rsp_breakpoint->setChecked(_config.rsp_filter.enable_breakpoint);

    if (ui.radioButton_req_settings->isChecked()) {
        ui.plainTextEdit_breakpoint_custom->setPlainText(
            QString::fromStdString(_config.req_filter.raw_custom_header_filter));
        ui.lineEdit_breakpoint_host->setText(
            QString::fromStdString(_config.req_filter.raw_host_filter));
    }
    else {
        ui.plainTextEdit_breakpoint_custom->setPlainText(
            QString::fromStdString(_config.rsp_filter.raw_custom_header_filter));
        ui.lineEdit_breakpoint_host->setText(
            QString::fromStdString(_config.rsp_filter.raw_host_filter));
    }

}

void QTFrontend::_save_config()
{

}


