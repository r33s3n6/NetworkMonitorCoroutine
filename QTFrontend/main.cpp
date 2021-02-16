#include "QTFrontend.h"
#include <QtWidgets/QApplication>

#include "../NetworkMonitorCoroutine/proxy_server.h"

#include <memory>

#include <boost/thread.hpp>

#include <QMetaType>



int main(int argc, char *argv[])
{
    qRegisterMetaType<QVector<int>>("QVector<int>");
    qRegisterMetaType<shared_ptr<session_info>>("shared_ptr<session_info>");
    
    qRegisterMetaType<size_t>("size_t");
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    QApplication a(argc, argv);
    



    proxy_tcp::proxy_server _backend_proxy_server("0.0.0.0","5559",16);
    boost::thread backend_thread(boost::bind(
        &proxy_tcp::proxy_server::start, &_backend_proxy_server));

 

    QTFrontend w(nullptr, _backend_proxy_server.get_display_filter());
    w.show();

    a.exec();
    backend_thread.join();
    return 0;
}
