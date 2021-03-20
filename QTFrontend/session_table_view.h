#pragma once

#include <QTableView>
#include <QKeyEvent>
#include "SessionDataModel.h"
#include <QSortFilterProxyModel>
class SessionTableView : public QTableView
{

	Q_OBJECT

public:
	SessionTableView(QWidget *parent);
	~SessionTableView();
	void set_original_model(QAbstractItemModel* model);
	void set_proxy_model(QSortFilterProxyModel* model) { _proxy_model = model; }
protected:
	SessionDataModel* _original_model;
	QSortFilterProxyModel* _proxy_model;
	void keyPressEvent(QKeyEvent* event);
};
