#include "session_table_view.h"
#include <iostream>
#include <vector>
using namespace std;

SessionTableView::SessionTableView(QWidget *parent)
	: QTableView(parent)
{
}

SessionTableView::~SessionTableView()
{
}

void SessionTableView::set_original_model(QAbstractItemModel* model)
{
	_original_model = (SessionDataModel*)model;

}

void SessionTableView::keyPressEvent(QKeyEvent* event)
{
	if (event->matches(QKeySequence::Delete)) {
		QModelIndexList selection = this->selectionModel()->selectedRows();
		vector<int> indexes;

		for (int i = 0; i < selection.count(); i++)
		{
			indexes.emplace_back(_proxy_model->mapToSource(selection.at(i)).row());
		}
		sort(indexes.begin(), indexes.end(), std::greater<int>());
		_original_model->removeRows(indexes);
		
	}


	QTableView::keyPressEvent(event);
}
