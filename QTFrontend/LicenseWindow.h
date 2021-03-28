#pragma once

#include <QWidget>
#include "ui_LicenseWindow.h"

class LicenseWindow : public QWidget
{
	Q_OBJECT

public:
	LicenseWindow(QWidget *parent = Q_NULLPTR);
	~LicenseWindow();


private:
	Ui::LicenseWindow ui;
	QString license_str;
};
