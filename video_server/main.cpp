#include <QCoreApplication>
#include <QSettings>
#include <QCoreApplication>
#include <QDebug>
#include "mainconsole.h"
#include "handle_video.h"
#include <QHostInfo>
int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    QSettings *ConfigWriter = new QSettings(QCoreApplication::applicationDirPath()+"/Config.ini",
                                            QSettings::IniFormat);
    if (ConfigWriter->value("/Path/Img/")==QVariant::Invalid
            ||ConfigWriter->value("/Path/Video/")==QVariant::Invalid
            ||ConfigWriter->value("/Listen/port/")==QVariant::Invalid){
        //当配置文件为空或不完整时,重新初始化文件
        cout<<"!!!!!!!配置信息不完整,初始化配置信息!!!!!!!!!!!"<<endl;
        ConfigWriter->setValue("/Path/Img/",QCoreApplication::applicationDirPath()+"/img/");
        ConfigWriter->setValue("/Path/Video/",QCoreApplication::applicationDirPath()+"/video/");
        ConfigWriter->setValue("/Listen/port/",5354);
    }
    delete ConfigWriter;

    QSettings *Config = new QSettings(QCoreApplication::applicationDirPath()+"/Config.ini",
                                            QSettings::IniFormat);
    cout<<"*****************************************"<<endl;
    cout<<"当前您的配置信息为:(有误请更改项目下的Config.ini文件)"<<endl;
    cout<<"接收图片帧保存地址:"<<Config->value("/Path/Img/").toString().toStdString()<<endl;
    cout<<"合成视频保存地址:"<<Config->value("/Path/Video/").toString().toStdString()<<endl;
    cout<<"监听端口:"<<Config->value("/Listen/port/").toString().toStdString()<<endl;
    //清空图片文件夹
    QDir dir(Config->value("/Path/Img/").toString());
    for(int i=0;i<dir.count();i++){
        dir.remove(dir[i]);
    }
    //获取主机名
    QString localHostName = QHostInfo::localHostName();
    //获取当前Ip地址
    QHostInfo info = QHostInfo::fromName(localHostName);
    //只输出IPv4地址（可能有多条
    foreach(QHostAddress address,info.addresses())
    {
        if(address.protocol() == QAbstractSocket::IPv4Protocol)

            cout <<"当前主机的IPV4地址: "<< address.toString().toStdString()<<endl;
    }

    cout<<"*****************************************"<<endl;

    //开启服务端监听
    mainconsole *server_listen =new mainconsole();
    int port = ConfigWriter->value("/Listen/port/").toInt();
    server_listen->start_listen(port);

    //开启图片结合视频线程
    handle_video *handle = new handle_video();
    handle->start();

    return a.exec();
}
