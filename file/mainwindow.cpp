#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QTableWidgetItem>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->rm_btn->setText("删除文件");
    ui->clean_btn->setText("清空");
    ui->tabWidget->setCurrentIndex(0);
    ui->tabWidget->setTabText(0, "发送端");
    ui->tabWidget->setTabText(1, "接收端");
    this->setWindowTitle("文件传输");
    ui->save_lab->setText("默认是当前的工作路径 \n" + QDir::currentPath());
    isStart = true;
    isOK = false;
    recvsize = 0;
    filesize = 0;
    filename.clear();
    save_path.clear();
    timer = new QTimer;
    udp_socket = new QUdpSocket;
    udp_socket->bind(PORT);
    tcpserver = new QTcpServer;
    tcpsocket = new QTcpSocket;
    tcp_server_socket = new QTcpSocket;

    tcpserver->listen(QHostAddress::Any, PORT+1);
    connect(timer, SIGNAL(timeout()), this, SLOT(time_out()));
    connect(tcpserver, SIGNAL(newConnection()),this, SLOT(tcp_connect()));
    connect(tcpsocket, SIGNAL(connected()), this, SLOT(tcp_socket_connect()));
    connect(tcpsocket, SIGNAL(readyRead()), this, SLOT(tcp_socket_recv()));
    connect(ui->ok_btn, SIGNAL(clicked(bool)), this, SLOT(click_send()));
    connect(ui->chouse_btn, SIGNAL(clicked(bool)), this, SLOT(click_file()));
    connect(ui->rm_btn, SIGNAL(clicked(bool)), this, SLOT(click_remove()));
    connect(ui->clean_btn, SIGNAL(clicked(bool)), this, SLOT(click_clean()));
    connect(ui->save_btn, SIGNAL(clicked(bool)), this, SLOT(click_save_path()));
    connect(ui->udp_btn, SIGNAL(clicked(bool)), this, SLOT(click_udp()));
    connect(udp_socket, SIGNAL(readyRead()), this, SLOT(recv_udp()));
    connect(ui->udp_clear, SIGNAL(clicked(bool)), this, SLOT(click_clear_udp_list()));

    QHostInfo info = QHostInfo::fromName(QHostInfo::localHostName());
    QString ip;
    foreach (QHostAddress address, info.addresses()) {
        if(address.protocol() == QAbstractSocket::IPv4Protocol)
            ip = address.toString();
    }
    ui->ip_lab->setText(QHostInfo::localHostName()+'\n'+ip);


}

MainWindow::~MainWindow()
{
    delete ui;
}
void MainWindow::send_file(QString file)// #filename#filesize#
{
    QFileInfo fileinfo(file);

    QString path = "#" + file.section('/', -1, -1) + "#" + QString::number(fileinfo.size())+"#";
    qDebug()<<"send_file path"<<path;
    qint64 len = tcpsocket->write(path.toUtf8());
    qDebug()<<"send_file path"<<path<<"len"<<len;
    qint64 file_len=0;
    qint64 file_all=0;
    QFile qfile(file);
    char buff[4*1024]={};
    if(len < 0)
    {
        qDebug()<<file<<"\n error";
    }
    else
    {
        #if 0
        int fd = open(file.toLatin1().data(),O_RDONLY);
        if(fd < 0)
        {
            qDebug()<<"open error";
            return;
        }


        while(file_len = read(fd, buff, sizeof(buff)))
        {
            qDebug()<<"send buff"<<buff;
            tcpsocket->write(buff,strlen(buff));
            file_all+=file_len;
            memset(buff, 0, sizeof(buff));
        }
#endif
        if(!qfile.open(QIODevice::ReadOnly))
        {
            QMessageBox::warning(this, "提示",QString("文件%1打开失败").arg(file));
            qfile.close();
            return;
        }
        do
        {
            memset(buff, 0, sizeof(buff));
            file_len=0;
            file_len = qfile.read(buff, sizeof(buff));
            file_len = tcpsocket->write(buff, file_len);
            file_all += file_len;

        }
        while(file_len > 0);
        if(file_all == qfile.size())
        {
            qDebug()<<"file_all is done "<<file_all;
            qfile.close();
//            isOK = false;
        }
    }
}
void MainWindow::time_out()//超时
{

    for(int i = 0; i < ui->listWidget->count(); i++)
    {
        //qDebug()<<"time_out"<<ui->listWidget->item(i)->text();

        send_file(ui->listWidget->item(i)->text());

    }
    timer->stop();
    //tcpsocket->disconnectFromHost();
    //tcpsocket->close();
}
void MainWindow::click_send()//点击确定
{
    qDebug()<<"click_send";
    if(!ui->listWidget->count())
    {
        QMessageBox::warning(this, "提示", "请添加文件");
    }
//    qDebug()<<ui->udp_tab->currentRow();
    if(ui->udp_tab->rowCount() && ui->udp_tab->currentRow() < 0)
    {
        ui->udp_tab->setCurrentCell(0,1);
    }
    qDebug()<<"tcpsocket "<<ui->udp_tab->currentItem()->text();
    tcpsocket->connectToHost(ui->udp_tab->currentItem()->text(), PORT+1);
}
void MainWindow::tcp_socket_recv()//客户端接收
{
    QString temp = tcpsocket->readAll();
    qDebug()<<"tcp_socket_recv"<<temp;
    if(temp == "recv is ok")
        isOK = true;

}
void MainWindow::click_file()//添加文件列表
{
    QStringList temp = QFileDialog::getOpenFileNames();
    if(!temp.isEmpty())
    {
        qDebug()<<temp;
        ui->listWidget->addItems(temp);
//        ui->listWidget->addItem(temp);
    }
//    QDesktopServices::openUrl(QUrl(qApp->applicationDirPath(),QUrl::TolerantMode));
}

void MainWindow::click_remove()//删除列表的文件
{
    if(!ui->listWidget->count())
    {
        QMessageBox::warning(this, "提示", "请添加删除文件");
        return;
    }
    if(ui->listWidget->currentRow() < 0)
    {
        QMessageBox::warning(this, "提示", "请选择要删除的文件");
        return;
    }
    else
    {
        ui->listWidget->takeItem(ui->listWidget->currentRow());
    }

}
void MainWindow::click_clean()//清空列表
{
    if(ui->listWidget->count())
        ui->listWidget->clear();
}

void MainWindow::click_save_path()//文件保存路径
{
    QString temp =  QFileDialog::getExistingDirectory();
    if(!temp.isEmpty())
    {
        save_path = temp;
        ui->save_lab->setText(save_path);
        qDebug()<<save_path;
    }

}

void MainWindow::click_udp()//UDP广播
{
    udp_socket->writeDatagram(QString("file is udp %1").arg(QHostInfo::localHostName()).toUtf8(), QHostAddress::Broadcast, PORT);
}
void MainWindow::click_clear_udp_list()//清空广播的列表
{
    ui->udp_tab->setRowCount(0);
    ui->udp_tab->clearContents();
}
void MainWindow::recv_udp()//UDP接收
{
    QByteArray array;
    QHostAddress address;
    quint16 port;
    QString recv_ip;
    array.resize(udp_socket->pendingDatagramSize());
    udp_socket->readDatagram(array.data(), array.size(), &address, &port);
    recv_ip = address.toString().section(':', -1, -1);
    qDebug()<<"array.data"<<array.data()<< recv_ip<<port;
    if(QString(array).section(' ', 0, 0) == "callback")
    {
        int row = ui->udp_tab->rowCount();
        ui->udp_tab->insertRow(row);
        ui->udp_tab->setItem(row, 1, new QTableWidgetItem(recv_ip));
        ui->udp_tab->setItem(row, 0, new QTableWidgetItem(QString(array).section(' ', -1, -1)));
        return;
    }
    int row = ui->udp_tab->rowCount();
    ui->udp_tab->insertRow(row);
    if(recv_ip == ui->ip_lab->text().section('\n', -1, -1))
    {
        qDebug()<<"is not deff";
        ui->udp_tab->setItem(row, 0, new QTableWidgetItem (QString(QHostInfo::localHostName()+" *MINE")));
    }
    else
    {
        qDebug()<<"\"else array.data()\""<<QString(array).section(' ',-1,-1);
        ui->udp_tab->setItem(row, 0, new QTableWidgetItem(QString(array).section(' ',-1,-1)));
        udp_socket->writeDatagram(QString("callback %1").arg(QHostInfo::localHostName()).toUtf8(), QHostAddress(recv_ip), port);
    }
    qDebug()<<"\"else array.data()\""<<QString(array).section(' ',-1,-1);
    ui->udp_tab->setItem(row, 1, new QTableWidgetItem(recv_ip));
}
void MainWindow::tcp_socket_connect()//tcp socket 链接
{
    qDebug()<<"tcp_socket_connect";
    timer->start(20);
}

void MainWindow::tcp_connect()//tcp server 链接
{
    tcp_server_socket = tcpserver->nextPendingConnection();
    connect(tcp_server_socket, SIGNAL(readyRead()), this,SLOT(tcp_server_recv()));
}
void MainWindow::tcp_server_recv()//服务端接收文件
{
    QByteArray buff = tcp_server_socket->readAll();

    if(isStart == true)
    {
REST:
        int i=0,j=0;
        QString temp;
        isStart = false;
        while(1)
        {
            if(buff.at(i)== '#')
            {
                j++;
            }
            if(j==3)
            {
                filesize=temp.toInt();
                if(!save_path.isEmpty())
                    filename = save_path + "/" +filename;
                break;
            }
            if(j == 1)
            {
                if(buff.at(i)!='#')
                    filename.append(buff.at(i));
            }
            else if(j == 2)
            {
                if(buff.at(i)!='#')
                    temp.append(buff.at(i));
            }
            i++;
        }
        buff = buff.remove(0, i+1);
        qDebug()<<"fileanme "<<filename<<"filesize"<<filesize<<"temp"<<temp<<"j = "<<j<<"i="<<i;

        if(filename.isEmpty())
        {
            qDebug()<<"filename is error";
        }

        recv_file.setFileName(filename);
        if(!recv_file.open(QIODevice::WriteOnly))
        {
            qDebug()<<"recv_file error";
            tcp_server_socket->disconnectFromHost();
            tcp_server_socket->close();
            return;
        }
        qDebug()<<"buff size"<< buff.size();
        if(filesize < (buff.size()+recvsize))
        {
            QByteArray file_other = buff.left(filesize - recvsize);
            buff = buff.remove(0,filesize - recvsize);
            if(0 < recv_file.write(file_other))
            {
                recv_file.close();
                filesize = 0;
                recvsize = 0;
                filename.clear();
                goto REST;
            }
        }
        qint64 len = recv_file.write(buff);
        if(len > 0)
        {
            recvsize += len;
            qDebug()<<"if -- recvsize "<<recvsize;
        }

        if(recvsize == filesize)
        {
            qDebug()<<"recvsize == filesize";
            tcp_server_socket->write("recv is ok");
            tcp_server_socket->disconnectFromHost();
            tcp_server_socket->close();
            filesize = 0;
            recvsize = 0;
            filename.clear();
            recv_file.close();
            isStart = true;
        }
    }
    else
    {
        if(filesize < (buff.size()+recvsize))
        {
            QByteArray file_other = buff.left(filesize - recvsize);
            buff = buff.remove(0, filesize - recvsize);
            if(0 < recv_file.write(file_other))
            {
                recv_file.close();
                filesize = 0;
                recvsize = 0;
                filename.clear();
                goto REST;
            }
        }
        qint64 len = recv_file.write(buff);
        if(len > 0)
        {
            recvsize += len;
            qDebug()<<"recvsize "<<recvsize;
        }
        if(recvsize == filesize)
        {
            qDebug()<<"recvsize == filesize";
            tcp_server_socket->write("recv is ok");
            recv_file.close();
            filename.clear();
            tcp_server_socket->disconnectFromHost();
            tcp_server_socket->close();
            filesize = 0;
            recvsize = 0;
            isStart = true;
        }

    }
}
