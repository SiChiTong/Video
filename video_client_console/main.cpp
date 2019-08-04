#include <QCoreApplication>
#include <QThread>
#include <QDebug>
#include "udp_thread.h"
#include <QQueue>
#include <QSettings>
#include <opencv2/opencv.hpp>
#include <iostream>
using namespace cv;
using namespace std;
double get_video_changesize(Mat src);
void handle_video(Mat fram,double change_size,QQueue<QByteArray>* que_img);
QByteArray GetbyteImg(Mat src);
QDateTime Last_send_time;
int send_fps;
int last_send_fps;
int send_flag;
int send_interval;
double current_size;
double last_size;
double thres;
bool isabnormal=false;
bool isShow = false;
int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    //开启线程
    QQueue<QByteArray> que_img;
    //相关数据初始化
    last_size=0;
    last_send_fps=0;
    send_fps=0;
    send_flag=0;
    int sleep=0;
    int chIn;
    QString ConfPath =QCoreApplication::applicationDirPath()+"/Config.ini";
    QSettings *ConfigWriter = new QSettings(QCoreApplication::applicationDirPath()+"/Config.ini",
                                            QSettings::IniFormat);
    if (ConfigWriter->value("/Server/Port/")==QVariant::Invalid
            ||ConfigWriter->value("/Path/Video/")==QVariant::Invalid
            ||ConfigWriter->value("/Server/Ip/")==QVariant::Invalid
            ||ConfigWriter->value("/Camera/RTSP/")==QVariant::Invalid
            |ConfigWriter->value("/Camera/RTSP/")==QVariant::Invalid){
        //当配置文件为空或不完整时,重新初始化文件
        ConfigWriter->setValue("/Path/Video/",QCoreApplication::applicationDirPath()+"/video/test.mp4");
        ConfigWriter->setValue("/Server/Ip/","127.0.0.0");
        ConfigWriter->setValue("/Server/Port/",5354);
        ConfigWriter->setValue("/Camera/RTSP/","rtsp://admin:hndx1234@192.168.254.2/Streaming/Channels/1");
    }
    string path = ((ConfigWriter->value("Path/Video")).toString()).toStdString();
    //释放配置器
    delete ConfigWriter;
    QSettings *Config = new QSettings(QCoreApplication::applicationDirPath()+"/Config.ini",
                                               QSettings::IniFormat);
       cout<<"*****************************************"<<endl;
       cout<<"当前您的配置信息为:(有误请更改项目下的Config.ini文件)"<<endl;
       cout<<"本地视频地址:"<<Config->value("/Path/Video/").toString().toStdString()<<endl;
       cout<<"服务端Ip:"<<Config->value("/Server/Ip/").toString().toStdString()<<endl;
       cout<<"服务端端口:"<<Config->value("/Server/Port/").toString().toStdString()<<endl;
       cout<<"摄像头RTSP地址:"<<Config->value("/Camera/RTSP/").toString().toStdString()<<endl;

       cout<<"*****************************************"<<endl;
       cout<<"请输入带宽等级(带宽越低,传输速度越慢)"<<endl;
       cout<<"1,峰值19k/s"<<endl;
       cout<<"2,峰值24k/s"<<endl;
       cout<<"3,峰值31k/s"<<endl;
       cout<<"4,峰值46k/s"<<endl;
       cout<<"5,峰值90k/s"<<endl;
       cout<<"6,无带宽限制"<<endl;
       cin>>chIn;
       while(1){
           if(chIn>=1&&chIn<=6){
               sleep=600-chIn*100;
               break;
           }
           else{
            cout<<"输入有误!请重新输入!"<<endl;
             cin>>chIn;
            continue;
           }
       }
        //开启线程
       udp_thread *udp = new udp_thread(&que_img,sleep);
       udp->start();

       VideoCapture capture;
       cout<<"请选择视频源:"<<endl;
       cout<<"1,文件"<<endl;
       cout<<"2,摄像头"<<endl;

       cin>>chIn;
       while(1){
           if(chIn==1){
               cout<<"文件模式"<<endl;
              capture.open(path);
               break;
           }
           else if(chIn==2){
                cout<<"摄像头模式"<<endl;
                cout<<"RTSP地址:"<<Config->value("/Camera/RTSP/").toString().toStdString()<<endl;
                capture.open(Config->value("/Camera/RTSP/").toString().toStdString());
                break;

           }
           else if(chIn==3){
               capture.open(0);
               break;
           }
           else{
            cout<<"输入有误!请重新输入!"<<endl;
             cin>>chIn;
            continue;
           }
       }

       cout<<"是否显示实时画面(需要在桌面环境下):"<<endl;
       cout<<"1,是"<<endl;
       cout<<"2,否"<<endl;
       cin>>chIn;
       while(1){
           if(chIn==1){
               isShow=true;
               break;
           }
           else{
               isShow=false;
            break;
           }
       }
        if (!capture.isOpened()) {
            printf("视频打开失败");
            return -1;
        }
        Mat fram;
        Mat bsmaskpMOG2;
        Mat kernel = getStructuringElement(MORPH_RECT, Size(3, 3), Point(-1, -1));
        //定义指针类型
        Ptr<BackgroundSubtractor> pMOG2 = createBackgroundSubtractorMOG2();
        while (capture.read(fram)) {
            if(isShow){
            imshow("input video", fram);
            }
            //进行背景消除 高斯模型
            pMOG2->apply(fram, bsmaskpMOG2);
            //形态学开操作 去噪声
            morphologyEx(bsmaskpMOG2, bsmaskpMOG2, MORPH_OPEN, kernel, Point(-1, -1));

            //处理当前帧
            handle_video(fram,get_video_changesize(bsmaskpMOG2),&que_img);

            if (isShow){
            imshow("MOG2", bsmaskpMOG2);
            }
            waitKey(30);
        }
        capture.release();

    return a.exec();
}
void handle_video(Mat fram,double current_size,QQueue<QByteArray>* que_img){

    //生成发送视频的帧率
    thres = current_size*10;
    //qDebug()<<"thres"<<thres;
    if(thres>=3){
        if(!isabnormal){
          cout<<"异物闯入"<<endl;
          cout<<"变化率:"<<current_size<<endl;
          send_fps=25;
        }
        isabnormal=true;
        //cout<<send_fps<<endl;
    }
    else{
        if(isabnormal){
           send_fps=20;
        }
        else{
            send_fps=0;
        }
        isabnormal=false;
    }

   //十分钟无异物闯入 更新一次
    if(QDateTime::currentDateTime().toTime_t()-Last_send_time.toTime_t()>=60*5){
        //0帧表示定时更新
         Last_send_time =QDateTime::currentDateTime();
         QByteArray simg =GetbyteImg(fram);
         //图片头部插入发送的帧数
         simg.insert(0,QString::number(10,10));                          //图片入队
         //在图片数据头部插入发送的时间
         //添加毫秒
         QString time = QString::number(Last_send_time.toTime_t(),10);
         int mecs=Last_send_time.time().msec();
         if (mecs>=0&&mecs<10){
             mecs=mecs*100;
         }
         else if(mecs>=10&&mecs<=99){
             mecs=mecs*10;
         }
         time+=QString::number(mecs,10);

          //注意这里在字节中插入整数时 需要将整数转为string类型 避免int型自动被转换为ascill
          simg.insert(0,time);
         que_img->enqueue(simg);
    }
       //当检测到有异物闯入 发送图片
    if(send_fps!=0){

         Last_send_time =QDateTime::currentDateTime();
        //获取图片的byte
         QByteArray simg =GetbyteImg(fram);
            //qDebug()<<"两次帧率不同发送"<<send_fps;
            //图片头部插入发送的帧数 ........

           simg.insert(0,QString::number(send_fps,10));                          //图片入队
            //在图片数据头部插入发送的时间
            //添加毫秒
            QString time = QString::number(Last_send_time.toTime_t(),10);
            int mecs=Last_send_time.time().msec();
            if (mecs>=0&&mecs<10){
                mecs=mecs*100;
            }
            else if(mecs>=10&&mecs<=99){
                mecs=mecs*10;
            }
            time+=QString::number(mecs,10);



             //注意这里在字节中插入整数时 需要将整数转为string类型 避免int型自动被转换为ascill
             simg.insert(0,time);
            que_img->enqueue(simg);
    }



    //将当前帧的值赋给上一帧
    last_size=current_size;
     last_send_fps=send_fps;
}

double get_video_changesize(Mat src){
    int height = src.rows;
    int width = src.cols;
    double sum = height*width;
    double change;
    QString b;
    int sum_change = 0;
    for (int col = 0; col <width; col++) {
        for (int row = 0; row < height; row++) {
            if (src.at<Vec3b>(row, col)[0] !=0|| src.at<Vec3b>(row, col)[1] != 0|| src.at<Vec3b>(row, col)[2] != 0) {
                sum_change++;
            }
        }

    }
    change = (sum_change / sum)*100;
    if (change >=0.01){
    return change;
    }
    else{
        return 0;
    }

}
QByteArray GetbyteImg(Mat src){
    //将Mat图片转byte函数
     cvtColor( src, src, CV_BGR2RGB ); //将图片转为qimg支持的通道模式
      //Mat 转Qimg
     QImage img = QImage((const unsigned char*)(src.data),
                         src.cols,
                         src.rows,QImage::Format_RGB888);
     //Qimg 转buffer
     QBuffer buffer;
         buffer.open(QIODevice::ReadWrite);
     img.save(&buffer,"jpg");
     //buffer转 Qbyte
      QByteArray pixArray;
             pixArray.append(buffer.data());
     return pixArray;
}
