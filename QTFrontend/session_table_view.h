#pragma once

#include <QTableView>
#include <QKeyEvent>


class SessionTableView : public QTableView
{

	Q_OBJECT

public:
	SessionTableView(QWidget *parent);
	~SessionTableView();

protected:
	void keyPressEvent(QKeyEvent* event);
};
