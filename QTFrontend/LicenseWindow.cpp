#include "LicenseWindow.h"

#include <fstream>
#include <iostream>
using namespace std;

LicenseWindow::LicenseWindow(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);

	QFile qf(":/QTFrontend/License.txt");

	if (!qf.open(QIODevice::ReadOnly)) {
		cout << "open of License file error" << endl;
	}
	else
	{
		license_str = qf.readAll();
	}

	qf.close();


	ui.plainTextEdit->setPlainText(license_str);


}

LicenseWindow::~LicenseWindow()
{

}
