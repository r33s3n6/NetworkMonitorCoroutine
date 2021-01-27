#include "QTFrontend.h"
#include <QtWidgets/QApplication>

#include "../NetworkMonitorCoroutine/proxy_server.h"
using namespace proxy_server;
#include <memory>

#include <boost/thread.hpp>

int main(int argc, char *argv[])
{

    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    QApplication a(argc, argv);
    QTFrontend w;
    w.show();



    proxy_server _backend_proxy_server("0.0.0.0","5559",16);
    boost::thread backend_thread(boost::bind(
        &proxy_server::start, &_backend_proxy_server));

    a.exec();
    backend_thread.join();
    return 0;
}
