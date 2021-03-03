#include "QTFrontend.h"
#include "qimage.h"




QTFrontend::QTFrontend(QWidget *parent,bool debug)
	: QMainWindow(parent),
	_backend_server(new proxy_server(&_display_filter,"config.dat")),
	_session_data(nullptr, &_display_filter),
	_config(_backend_server->get_config_ptr())
{
	ui.setupUi(this);
	
	_backend_thread=make_shared<boost::thread>(boost::bind(
		&proxy_tcp::proxy_server::start, _backend_server));

	_display_config();
	//_set_config();//可视化config

	if (!debug) {
		ui.menuDebug->setEnabled(false);
	}

	_setup_table();
	//table settings start
 

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

	connect(ui.pushButton_restart_proxy_server, &QPushButton::clicked, this, &QTFrontend::_restart_backend_server);
	connect(ui.pushButton_trust_root_ca, &QPushButton::clicked, this, &QTFrontend::_trust_root_ca);

	auto edited_handler = //deprecated
		[this](bool modified) {
		auto _session_info_ptr = _session_data.get_session_info_ptr(_display_id);

		_session_info_ptr->edited = true;
	};
	//connect(ui.plaintext_req_text, &QPlainTextEdit::modificationChanged, edited_handler);
	//connect(ui.plaintext_rsp_text, &QPlainTextEdit::modificationChanged, edited_handler);

	auto tab_changed_handler = 
		[this](int new_tab,bool is_req) {
		auto _session_info_ptr = _session_data.get_session_info_ptr(_display_id);
		if (_session_info_ptr->edited) {
			auto hex_editor = is_req ? ui.hexEdit_req : ui.hexEdit_rsp;
			auto text_editor = is_req ? ui.plaintext_req_text : ui.plaintext_rsp_text;

			if (new_tab == 0) {//to display data
				auto& display_data = is_req_intercepted ? *_session_info_ptr->req_data_for_display : *_session_info_ptr->rsp_data_for_display;
				display_data = _raw_chunk_to_text(hex_editor->data().toStdString());
				text_editor->setPlainText(QString::fromStdString(display_data));

			}
			//_session_info_ptr->edited = false;
		}
		

	};
	//connect(ui.tab_request, &QTabWidget::currentChanged, [=](int x) {tab_changed_handler(x, true); });
	//connect(ui.tab_response, &QTabWidget::currentChanged, [=](int x) {tab_changed_handler(x, false); });
	
	//connect(ui.lineEdit_breakpoint_host, &QLineEdit::editingFinished, this, &QTFrontend::_set_config);
	//connect(ui.plainTextEdit_breakpoint_custom, &QPlainTextEdit::finish, this, &QTFrontend::_set_config);
	

	ui.scrollArea->setVisible(true);

	_activate_breakpoint_box(false);
	_set_filter_vec(true);
	_set_filter_vec(false);
	ui.statusBar->showMessage("TODO:Status Bar");
	ui.label_server_restart_required->hide();
}


void QTFrontend::_setup_table() {
	_proxy_session_data.setSourceModel(&_session_data);

	ui.table_session->setModel(&_proxy_session_data);
	ui.table_session->sortByColumn(0, Qt::AscendingOrder);

	for (int i = 0; i < sizeof(_config->column_width) / sizeof(int); i++) {
		ui.table_session->setColumnWidth(i, _config->column_width[i]);
	}
	ui.table_session->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
	ui.table_session->setTextElideMode(Qt::ElideMiddle);
	ui.table_session->setWordWrap(true);
	ui.table_session->show();
	connect(ui.table_session, &QTableView::customContextMenuRequested, this, &QTFrontend::_display_table_context_menu);

	table_session_context_menu = new QMenu(this);
	

	table_session_context_menu->addAction(ui.action_edit_and_replay);
	table_session_context_menu->addAction(ui.action_replay_session);
	table_session_context_menu->addSeparator();
	connect(ui.action_edit_and_replay, &QAction::triggered, this, &QTFrontend::_replay_session_with_bp);
	connect(ui.action_replay_session, &QAction::triggered, this, &QTFrontend::_replay_session);

	table_session_bp_menu = new QMenu(this);
	table_session_context_menu->addMenu(table_session_bp_menu);

	//table_session_bp_menu->setTitle(QString::fromLocal8Bit("www.example.com 断点设置"));
	table_session_bp_menu->setTitle("www.example.com 断点设置");
	table_session_bp_menu->addAction(ui.action_req_bp);
	table_session_bp_menu->addAction(ui.action_rsp_bp);
	table_session_bp_menu->addAction(ui.action_both_bp);

	connect(ui.action_req_bp, &QAction::triggered, this, &QTFrontend::_add_req_bp);
	connect(ui.action_rsp_bp, &QAction::triggered, this, &QTFrontend::_add_rsp_bp);
	connect(ui.action_both_bp, &QAction::triggered, this, &QTFrontend::_add_both_bp);

	//connect(this, &QTFrontend::session_replayed, &_session_data, &SessionDataModel::session_replayed);
	//table_session_context_menu->addAction();
	//table_session_context_menu->addAction();
	table_session_context_menu->addSeparator();
	//table settings end

}


void QTFrontend::_add_bp(bool is_req) {
	_set_config();
	auto& filter = is_req ? _config->req_filter : _config->rsp_filter;
	if (_context_menu_session->host.size() > 0) {
		//若之前没启动断点，也不应该把旧的断点filter启用 TODO:可开关feature
		if (!filter.enable_breakpoint) {//没启用的情况下，清空数据
			filter.raw_custom_header_filter.clear();
			filter.raw_host_filter.clear();
		}
		if (filter.raw_host_filter.size() == 0) {
			filter.raw_host_filter = _context_menu_session->host;
		}
		else {
			if (filter.raw_host_filter.find(_context_menu_session->host) == string::npos) {
				if (*filter.raw_host_filter.rbegin() != ';')
					filter.raw_host_filter.append(";");
				filter.raw_host_filter.append(_context_menu_session->host);
			}
				
		}
		filter.enable_breakpoint = true;
	}
	_set_filter_vec(is_req);
	_display_config();
}

void QTFrontend::_delete_bp(bool is_req) {
	_set_config();
	auto& filter = is_req ? _config->req_filter : _config->rsp_filter;

	if (_context_menu_session->host.size() > 0) {
		size_t pos = filter.raw_host_filter.find(_context_menu_session->host);
		if (pos != string::npos) {

			size_t end_splitter_pos = pos + _context_menu_session->host.size();
			if (end_splitter_pos >= filter.raw_host_filter.size()
				|| filter.raw_host_filter[end_splitter_pos] != ';')
				filter.raw_host_filter.erase(pos, _context_menu_session->host.size());
			else
				filter.raw_host_filter.erase(pos, _context_menu_session->host.size() + 1);

			
		}


		//filter.enable_breakpoint = true;
	}
	_set_filter_vec(is_req);
	_display_config();
}







void QTFrontend::_display_table_context_menu(QPoint pos) {

	auto row = ui.table_session->indexAt(pos).row();
	if (row == -1) {
		table_session_context_menu->setEnabled(false);
		_context_menu_session = nullptr;
	}
	else {
		table_session_context_menu->setEnabled(true);
		auto id = _proxy_session_data.data(_proxy_session_data.index(row, 0)).toInt();// 获取实际位置
		_context_menu_session = _session_data.get_session_info_ptr(id);

		ui.action_req_bp->setChecked(
			_config->req_filter.raw_host_filter.
			find(_context_menu_session->host) != string::npos);

		ui.action_rsp_bp->setChecked(
			_config->rsp_filter.raw_host_filter.
			find(_context_menu_session->host) != string::npos);

		ui.action_both_bp->setChecked(ui.action_req_bp->isChecked() && ui.action_rsp_bp->isChecked());

		//table_session_bp_menu->setTitle(QString::fromStdString(_context_menu_session->host) + QString::fromLocal8Bit(" 断点设置"));
		table_session_bp_menu->setTitle(QString::fromStdString(_context_menu_session->host) + QString(" 断点设置"));
	}
	
	table_session_context_menu->popup(ui.table_session->viewport()->mapToGlobal(pos));
}

void QTFrontend::_replay_session_with_bp()
{
	_replay_session(true);
}

void QTFrontend::_replay_session(bool with_bp)
{
	

	if (_context_menu_session->protocol == "https")
		_backend_server->replay(_context_menu_session->raw_req_data, with_bp, true);//非阻塞
	else if (_context_menu_session->protocol == "http")
		_backend_server->replay(_context_menu_session->raw_req_data, with_bp, false);//非阻塞
	else
		return;//websocket 不重放


}

void QTFrontend::_save_data_to_hexeditor(shared_ptr<session_info> _session_info_ptr) { //deprecated
	
	//<delete>TODO 连同上面的一起，这里面的换行符似乎有问题</delete>
	auto hex_editor = is_req_intercepted ? ui.hexEdit_req : ui.hexEdit_rsp;
	auto text_editor = is_req_intercepted ? ui.plaintext_req_text : ui.plaintext_rsp_text;
	auto tab_widget = is_req_intercepted ? ui.tab_request : ui.tab_response;
	auto& display_data = is_req_intercepted ? *_session_info_ptr->req_data_for_display : *_session_info_ptr->rsp_data_for_display;
	if (tab_widget->currentIndex() == 1) {//raw data now

		display_data = _raw_chunk_to_text(hex_editor->data().toStdString());
		text_editor->setPlainText(QString::fromStdString(display_data));

	}
	else if (tab_widget->currentIndex() == 0) {//text data now

		hex_editor->setData(text_editor->toPlainText().toLatin1());

	}

		
	
}

void QTFrontend::display_full_info(const QModelIndex& index, const QModelIndex& prev) {
	
	if (prev.row() != -1) {
		ui.table_session->setRowHeight(prev.row(), ui.table_session->rowHeight(index.row()));
	}
	ui.table_session->resizeRowToContents(index.row());

	auto _session_info_ptr = _session_data.get_session_info_ptr(_display_id);

	
	//if (_session_info_ptr->edited) {//暂存修改
	//	_save_data_to_hexeditor(_session_info_ptr);
		
	//}
	if (_session_info_ptr->send_behaviour == intercept || _session_info_ptr->receive_behaviour == intercept) {
		auto& temp_data = is_req_intercepted ? _session_info_ptr->temp_req_data : _session_info_ptr->temp_rsp_data;
		auto hex_editor = is_req_intercepted ? ui.hexEdit_req : ui.hexEdit_rsp;
		temp_data = string(hex_editor->data().constData(), hex_editor->data().length());

		auto& display_data = is_req_intercepted ? *_session_info_ptr->req_data_for_display : *_session_info_ptr->rsp_data_for_display;
		display_data = _raw_chunk_to_text(temp_data);
	}
	



	_display_id = _proxy_session_data.data(_proxy_session_data.index(index.row(), 0)).toInt();// 获取新的实际位置

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
	//TODO potential design problem: is_req_intercepted 不应该由单独变量管理
	_activate_editor(false, is_req_intercepted);//_disable_editor of request/response 

	//根据当前tab和是否被编辑过决定发送哪个内容，
	//TODO应该加入一个还原数据按钮


	//if (_session_info_ptr->edited) {
	//if (_session_info_ptr->edited) {

		//_save_data_to_hexeditor(_session_info_ptr);
		//_session_info_ptr->edited = false;//reset for response
	//}
	auto hex_editor = is_req_intercepted ? ui.hexEdit_req : ui.hexEdit_rsp;
	auto text_editor = is_req_intercepted ? ui.plaintext_req_text : ui.plaintext_rsp_text;


	auto& display_data = is_req_intercepted ? *_session_info_ptr->req_data_for_display : *_session_info_ptr->rsp_data_for_display;
	display_data = _raw_chunk_to_text(hex_editor->data().toStdString());
	text_editor->setPlainText(QString::fromStdString(display_data));


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

string QTFrontend::_raw_chunk_to_text(const string& data) {
	size_t split_pos = data.size();
	if (_http_integrity_check(make_shared<string>(data), split_pos) != integrity_status::chunked) {
		return data;
	}
	string ret;
	ret.append(data.substr(0, split_pos));

	//不检查报文完整性
	while (split_pos<data.size()) {
		size_t _chunk_length_end_pos =
			data.find("\r\n",split_pos);

		if (_chunk_length_end_pos == string::npos)
			return ret;

		//get length
		size_t body_length = hex2decimal(data.substr(split_pos, _chunk_length_end_pos- split_pos).c_str());

		size_t body_end_pos = _chunk_length_end_pos + 2 + body_length;

		ret.append(data.substr(_chunk_length_end_pos + 2, body_end_pos - (_chunk_length_end_pos + 2)));
		split_pos = body_end_pos + 2;//正好最后一个\r\n的后面一位



		
		
	}
	//if (ret.size() > data.size())
	//	throw "fuck";
	return ret;
	

};

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
		//ui.plaintext_req_text->setReadOnly(!active);
	}
	else {
		ui.hexEdit_rsp->setReadOnly(!active);
		//ui.plaintext_rsp_text->setReadOnly(!active);
	}
	

}





//TODO: 增加保存文件功能
void QTFrontend::_display_full_info(size_t display_id)
{
	

	

	auto _session_info_ptr = _session_data.get_session_info_ptr(display_id);

	if (_session_info_ptr->send_behaviour == intercept) {
		ui.label_intercept->setText("Request has been intercepted");
		_activate_breakpoint_box(true);
		_activate_editor(_session_info_ptr->req_completed, true);//_activate_editor of request	
		
		is_req_intercepted = true;
		
	}
	else if (_session_info_ptr->receive_behaviour == intercept) {
		ui.label_intercept->setText("Response has been intercepted");
		_activate_breakpoint_box(true);
		_activate_editor(_session_info_ptr->rsp_completed, false);//_activate_editor of response
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
		//处理暂存的更改
		if (_session_info_ptr->send_behaviour == intercept &&_session_info_ptr->temp_req_data.size()>0 ) {
			ui.hexEdit_req->setData(
				QByteArray(_session_info_ptr->temp_req_data.c_str(),
					_session_info_ptr->temp_req_data.size()));

		}
		else {
			ui.hexEdit_req->setData(
				QByteArray(_session_info_ptr->raw_req_data->c_str(),
					_session_info_ptr->raw_req_data->size()));
		}
		
	}
	else {
		ui.hexEdit_req->setData(QByteArray());
	}

	if (_session_info_ptr->raw_rsp_data) {
		//处理暂存的更改
		if (_session_info_ptr->receive_behaviour == intercept && _session_info_ptr->temp_rsp_data.size() > 0) {
			ui.hexEdit_rsp->setData(
				QByteArray(_session_info_ptr->temp_rsp_data.c_str(),
					_session_info_ptr->temp_rsp_data.size()));
		}
		else {

		
		ui.hexEdit_rsp->setData(
			QByteArray(_session_info_ptr->raw_rsp_data->c_str(),
				_session_info_ptr->raw_rsp_data->size()));
		}
	}
	else {
		ui.hexEdit_rsp->setData(QByteArray());
	}

	if (!(_session_info_ptr->rsp_data_for_display)) {
		ui.plaintext_rsp_text->setPlainText(QString());
		return;
	}
	
	if (_session_info_ptr->failed) {
		ui.plaintext_rsp_text->setPlainText(
			QString::fromLocal8Bit(_session_info_ptr->rsp_data_for_display->c_str()));//Windows Api return char with local codeset
	}
	else {
		ui.plaintext_rsp_text->setPlainText(
			QString::fromStdString(*(_session_info_ptr->rsp_data_for_display)));
	}
	
	//ui.plaintext_rsp_text->setPlainText(
	//	QString::fromStdString(*(_session_info_ptr->rsp_data_for_display)));

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



void QTFrontend::_set_enable_config() {
	_config->req_filter.enable_breakpoint =
		ui.checkBox_enable_req_breakpoint->isChecked();

	_config->rsp_filter.enable_breakpoint =
		ui.checkBox_enable_rsp_breakpoint->isChecked();
}

//save all config to _config, please call it carefully due to performance
void QTFrontend::_set_config()
{
	_set_enable_config();
	
	bool is_req = ui.radioButton_req_settings->isChecked();
	auto& filter = is_req ? _config->req_filter : _config->rsp_filter;

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
		_set_filter_vec(is_req);

	_config->port = ui.lineEdit_port->text().toStdString();

	_config->secondary_proxy = ui.lineEdit_2nd_proxy->text().toStdString();
	_config->ssl_decrypt = ui.checkBox_https_decrypt->isChecked();


	_config->allow_lan_conn = ui.checkBox_allow_lan->isChecked();
	_config->system_proxy = ui.checkBox_system_proxy->isChecked();
	_config->verify_server_certificate = ui.checkBox_verify_certificate->isChecked();

	_config->save_config("config.dat");
	
}

void QTFrontend::_restart_backend_server()
{
	_set_config();
	_config->save_config();
	if (_backend_server) {
		_backend_server->stop();
	}
	_backend_server = make_shared<proxy_server>(&_display_filter,"config.dat");
	_config = _backend_server->get_config_ptr();
	_backend_thread = make_shared<boost::thread>(boost::bind(
		&proxy_tcp::proxy_server::start, _backend_server));
	ui.label_server_restart_required->hide();
	_display_config();
}

void QTFrontend::_trust_root_ca()
{
	system(string("certutil -addstore -f -enterprise -user root ./cert/CAcert.pem").c_str());
}

void QTFrontend::_set_filter_vec(bool is_req) {

	breakpoint_filter& filter = is_req ? _config->req_filter : _config->rsp_filter;

	shared_ptr<vector<string>> header_vec_ptr
		= string_split(filter.raw_custom_header_filter, "\r\n");

	if(filter.raw_host_filter.size()>0)
		header_vec_ptr->emplace_back("host:"+filter.raw_host_filter);
	
	filter.header_filter_vec.clear();


	for (auto header : *header_vec_ptr) {
		size_t pos = header.find(":");
		if (pos == string::npos || (pos + 1) == header.size())
			continue;
		http_header http_header;
		http_header.key = string_trim(header.substr(0, pos));
		transform(http_header.key.begin(), http_header.key.end(), http_header.key.begin(), ::tolower);

		shared_ptr<vector<string>> value_vec_ptr = string_split(header.substr(pos + 1, header.size() - pos - 1),";");
		for (auto v : *value_vec_ptr) {
			string&& temp_v = string_trim(v);
			if (temp_v.size() > 0) {
				http_header.value.emplace_back(temp_v);
				//cout << temp_v << endl;//DEBUG
			}
				
		}

		filter.header_filter_vec.emplace_back(http_header);

	}
}

void QTFrontend::_toggle_breakpoint_config()
{
	
	if (ui.radioButton_req_settings->isChecked() ^ last_breakpoint_req_checked) {//不同
		

		if (ui.radioButton_req_settings->isChecked()) {
			_config->rsp_filter.raw_custom_header_filter = ui.plainTextEdit_breakpoint_custom->toPlainText().toStdString();
			_config->rsp_filter.raw_host_filter = ui.lineEdit_breakpoint_host->text().toStdString();
		}
		else {
			_config->req_filter.raw_custom_header_filter = ui.plainTextEdit_breakpoint_custom->toPlainText().toStdString();
			_config->req_filter.raw_host_filter = ui.lineEdit_breakpoint_host->text().toStdString();

		}
		_display_config();


		last_breakpoint_req_checked = ui.radioButton_req_settings->isChecked();
	}
	//相同不作处理
	   
		
	
	

	
	

}

void QTFrontend::_display_config()
{
	ui.checkBox_enable_req_breakpoint->setChecked(_config->req_filter.enable_breakpoint);
	ui.checkBox_enable_rsp_breakpoint->setChecked(_config->rsp_filter.enable_breakpoint);

	if (ui.radioButton_req_settings->isChecked()) {
		ui.plainTextEdit_breakpoint_custom->setPlainText(
			QString::fromStdString(_config->req_filter.raw_custom_header_filter));
		ui.lineEdit_breakpoint_host->setText(
			QString::fromStdString(_config->req_filter.raw_host_filter));
	}
	else {
		ui.plainTextEdit_breakpoint_custom->setPlainText(
			QString::fromStdString(_config->rsp_filter.raw_custom_header_filter));
		ui.lineEdit_breakpoint_host->setText(
			QString::fromStdString(_config->rsp_filter.raw_host_filter));
	}
	
	ui.lineEdit_port->setText(QString::fromStdString(_config->port));
	ui.lineEdit_2nd_proxy->setText(QString::fromStdString(_config->secondary_proxy));
	ui.checkBox_https_decrypt->setChecked(_config->ssl_decrypt);
	
	ui.checkBox_allow_lan->setChecked(_config->allow_lan_conn);
	ui.checkBox_system_proxy->setChecked(_config->system_proxy);
	ui.checkBox_verify_certificate->setChecked(_config->verify_server_certificate);

}




