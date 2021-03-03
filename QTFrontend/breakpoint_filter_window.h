#pragma once
//deprecated

#include <QWidget>
#include "ui_breakpoint_filter_window.h"

class breakpoint_filter_window : public QWidget
{
	Q_OBJECT

public:
	breakpoint_filter_window(QWidget *parent = Q_NULLPTR);
	~breakpoint_filter_window();

private:
	Ui::breakpoint_filter_window ui;
};
