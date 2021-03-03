#include "QTFrontend.h"
#include <QtWidgets/QApplication>

#include "../NetworkMonitorCoroutine/proxy_server.h"

#include <memory>

#include <boost/thread.hpp>

#include <QMetaType>

//TODO:
/*
* save_session
* 
*/


//TODO 过滤器：过滤某些host，meta-type 显示上/断点上
//TODO stream truncated
//TODO 前后端解耦更加彻底些，后续可以加上webui,做成远程调试器

//TODO 更好的更改头的办法

//TODO 删除所有session，粗体标注未被查看的session


int main(int argc, char *argv[])
{
    qRegisterMetaType<QVector<int>>("QVector<int>");
    qRegisterMetaType<shared_ptr<session_info>>("shared_ptr<session_info>");
    
    qRegisterMetaType<size_t>("size_t");
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    QApplication a(argc, argv);
    




    //proxy_tcp::proxy_server _backend_proxy_server();
    //boost::thread backend_thread(boost::bind(
    //    &proxy_tcp::proxy_server::start, &_backend_proxy_server));

 

    
    bool debug = false;
    if (argc > 1) {
        if (string(argv[1]) == "--debug") {
            AllocConsole();
            freopen("CON", "w", stdout);
            debug = true;
        }
    }
    
    QTFrontend w(nullptr,debug);
    w.show();

    a.exec();
    //backend_thread.join();
    return 0;
}
