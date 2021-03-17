#include "session_table_view.h"
#include <iostream>
using namespace std;

SessionTableView::SessionTableView(QWidget *parent)
	: QTableView(parent)
{
}

SessionTableView::~SessionTableView()
{
}

void SessionTableView::keyPressEvent(QKeyEvent* event)
{
	if (event->matches(QKeySequence::Delete)) {
		cout << "delete !! " << endl;
	}


	QTableView::keyPressEvent(event);
}
