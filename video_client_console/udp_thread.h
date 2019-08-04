#ifndef UDP_THREAD_H
#define UDP_THREAD_H

#include <QObject>
#include <QUdpSocket>
#include <QThread>
#include <QSettings>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <QTimer>
#include <QDateTime>
#include <QBuffer>
#include <string.h>
#include <QQueue>
#include <QCoreApplication>
#include <QPixmap>
using namespace std;
class udp_thread : public QThread
{
    Q_OBJECT
    const int UDP_MAX_SIZE=512;  //定义udp报文最大大小
    //包头
    struct PackageHeader
    {
        //包头大小(sizeof(PackageHeader))
        unsigned int uTransPackageHdrSize;
        //当前包的大小(sizeof(PackageHeader)+数据长度)
        unsigned int uTransPackageSize;
        //数据的总大小
        unsigned int uDataSize;
        //数据被分成包的个数
        unsigned int uDataPackageNum;
        //数据包当前的帧号
        unsigned int uDataPackageCurrIndex;
        //数据包在整个数据中的偏移
        unsigned int uDataPackageOffset;
        //发送时间
        unsigned long SendTime1;
        unsigned long SendTime2;
        //发送帧率
        unsigned int Fps;

    };
public:
    explicit udp_thread(QObject *parent = 0);
    udp_thread(QQueue<QByteArray> *,int slee);
signals:
private:
    void run();
    QSettings *ConfigReader;  //配置写入读入指针
    QSettings *ConfigWriter;
    QString send_time;
    QString re_send_time;
    QString re_id;
    QString ConfPath;
    string path;
    QString Fps;
    QHostAddress ip;
    quint16 port;
    QPixmap pixmap;
    QQueue<QByteArray> *que_img;
    QQueue<QByteArray> *re_que_img;
    QByteArray img;
    QByteArray re_img;
    void divide_package(QByteArray img);
    void save_img_queu(QByteArray img);
    QUdpSocket  *udpsocketSend;
    QString split_char = "#";
    int sleep;
public slots:
    void ReadPendingDatagrams();
};

#endif // UDP_THREAD_H
