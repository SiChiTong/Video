#include "udp_thread.h"

udp_thread::udp_thread(QObject *parent) : QThread(parent)
{

}
udp_thread::udp_thread(QQueue<QByteArray> *que,int slee){
    que_img=que;
    udpsocketSend=new QUdpSocket(this);
    udpsocketSend->bind(QHostAddress::AnyIPv4,5453);
    //监听消息的传入
    connect(udpsocketSend,SIGNAL(readyRead()),this,SLOT(ReadPendingDatagrams()));
    ConfPath +=QCoreApplication::applicationDirPath()+"/Config.ini";
    sleep = slee;
}
void udp_thread::ReadPendingDatagrams(){
    QByteArray datagram;
   while (udpsocketSend->hasPendingDatagrams())
   {

       //接收信息 处理丢包重发
          datagram.resize(udpsocketSend->pendingDatagramSize());
          QHostAddress sender;
          quint16 senderPort;
          udpsocketSend->readDatagram(
                      datagram.data(),
                      datagram.size(),
                      &sender,
                      &senderPort);
          cout<<"***************************************************"<<endl;
          QString Msg;
          Msg.prepend(datagram);
          QStringList msglist = Msg.split("#");
          re_id =msglist[1];
          if(msglist[0]=="RESEND"){
             cout<<"收到重发消息:"<<sender.toString().toStdString()<<":"<<senderPort<<endl;
             cout<<"重发的帧号:"<<re_id.toStdString()<<endl;
             while(re_que_img->empty()){
                 //不为空时出队列一个
                 re_img = que_img->dequeue();
                 //获取图片头部时间
                 re_send_time = re_img.mid(0,13);
                 re_img.remove(0,13);
                 //提取FPs
                 Fps=re_img.mid(0,2);
                 re_img.remove(0,2);



             }
             cout<<"************重发帧号:"<<re_id.toStdString()<<"成功****************"<<endl;
          }


   }
}
void udp_thread::run(){
    while (1) {
        //循环间歇 避免过快
        QThread::msleep(30);
        while(!que_img->isEmpty()){
            //不为空时出队列一个
            img = que_img->dequeue();
            //save_img_queu(img);
            //获取图片头部时间
            send_time = img.mid(0,13);
            img.remove(0,13);
            cout<<"***************************************************"<<endl;
            cout<<"发送帧号:"<<send_time.toStdString()<<"中............."<<endl;
            //提取FPs
            Fps=img.mid(0,2);
            img.remove(0,2);
             divide_package(img);
              QThread::msleep(300);
        }
    }
}
//void udp_thread::save_img_queu(QByteArray img){
//    //用来存放已发送的三帧图片 便于丢包重发处理 的函数
//    //只存储最近3帧图片
//    int size = re_que_img->size();
//    if (size<3){
//        //当队列中的图片数小于3 队列不满
//        re_que_img->enqueue(img);
//    }
//    else{
//        //出队列一个
//        re_que_img->dequeue();
//        //入队列
//        re_que_img->enqueue(img);
//    }
//}
void udp_thread::divide_package(QByteArray img){
    //拆包函数 udp应用层协议
    //图片转为pixmap类型
    QBuffer buffer;
    buffer.setBuffer(&img);
    //分包开始+++++++++++++++++++++++++++
    int dataLength=buffer.data().size();
    unsigned char *dataBuffer=(unsigned char *)buffer.data().data();

    int packetNum = 0;
     //最后一个包的报文大小
    int lastPaketSize = 0;
    //计算拆分后的总报文个数 一张图片所有字节个数/udp一个报文最大个数
    packetNum = dataLength / UDP_MAX_SIZE;
    //计算最后一个报文的大小
    lastPaketSize = dataLength % UDP_MAX_SIZE;
    //正在处理的当前报文的报文号
    int currentPacketIndex = 0;
     //当最后一个数据报文不为空时
    if (lastPaketSize != 0)
    {
        //报文数量+1
        packetNum = packetNum + 1;
    }
     //写包头
    PackageHeader packageHead;
    //设置包头的大小
    packageHead.uTransPackageHdrSize=sizeof(packageHead);
    //设置data大小
    packageHead.uDataSize = dataLength;
    //设置拆分的数据包个数
    packageHead.uDataPackageNum = packetNum;
    //设置发送的时间
    packageHead.SendTime1=send_time.mid(0,6).toLong();
    //qDebug()<<"sendtime1"<<packageHead.SendTime1;
    packageHead.SendTime2=send_time.mid(6,7).toLong();
    //qDebug()<<"sendtime2"<<packageHead.SendTime2;
    //设置发送帧率
    packageHead.Fps=Fps.toInt();

    //定义数据包
    unsigned char frameBuffer[1024*1024];
     //清空数组
    memset(frameBuffer,0,1024*1024);
    //循环生成每个数据包
    //qDebug()<<"包的大小"<<dataLength;
    //qDebug()<<"包的个数"<< packetNum;
    while (currentPacketIndex < packetNum)
    {
        //cout<<"处理的当前包号:"<<currentPacketIndex<<endl;

       //当不为最后一个数据包时
        if (currentPacketIndex < (packetNum-1))
        {
            //当前包头的大小(sizeof(PackageHeader)+当前数据包长度)
            packageHead.uTransPackageSize = sizeof(PackageHeader)+UDP_MAX_SIZE;
            //当前数据包的序号
            packageHead.uDataPackageCurrIndex = currentPacketIndex+1;
            //当前数据包的偏移 (当前数据包的序号-1)*每个数据包的大小
            packageHead.uDataPackageOffset = currentPacketIndex*UDP_MAX_SIZE;
            //拷贝包头到 空包中
           //memcpy 数用于 把资源内存（src所指向的内存区域） 拷贝到目标内存（dest所指向的内存区域）；拷贝多少个？有一个size变量控制
            memcpy(frameBuffer, &packageHead, sizeof(PackageHeader));
            //拷贝数据包到 包中
            //其中拷贝的地址为 frameBuffer+包头的大小(地址位移) 的后面
            //第二个为数据包+数据包的偏移大小
            memcpy(frameBuffer+sizeof(PackageHeader), dataBuffer+packageHead.uDataPackageOffset, UDP_MAX_SIZE);
            //发送数据
            ConfigReader=new QSettings(ConfPath,QSettings::IniFormat);
            ip = QHostAddress(ConfigReader->value("/Server/Ip").toString());
            port =ConfigReader->value("/Server/Port").toString().toUInt();

            udpsocketSend->writeDatagram(
                                    (const char*)frameBuffer, packageHead.uTransPackageSize,
                                     ip, port);
            //读入入完成后删除指针
            delete ConfigReader;


            currentPacketIndex++;
            //延时40ms
            QThread::msleep(sleep);

        }
        else
        {
            //处理最后一个数据包
            packageHead.uTransPackageSize = sizeof(PackageHeader)+(dataLength-currentPacketIndex*UDP_MAX_SIZE);
            packageHead.uDataPackageCurrIndex = currentPacketIndex+1;
            packageHead.uDataPackageOffset = currentPacketIndex*UDP_MAX_SIZE;
            memcpy(frameBuffer, &packageHead, sizeof(PackageHeader));
            memcpy(frameBuffer+sizeof(PackageHeader), dataBuffer+packageHead.uDataPackageOffset, dataLength-currentPacketIndex*UDP_MAX_SIZE);
            ConfigReader=new QSettings(ConfPath,QSettings::IniFormat);
            ip = QHostAddress(ConfigReader->value("/Server/Ip").toString());
            port =ConfigReader->value("/Server/Port").toString().toUInt();
            udpsocketSend->writeDatagram(
                                    (const char*)frameBuffer, packageHead.uTransPackageSize,
                                    ip, port);
            //读入入完成后删除指针
            delete ConfigReader;
            QThread::msleep(sleep);
            currentPacketIndex++;
        }
    }
}
