#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpServer>
#include <QFileDialog>
#include <QMessageBox>
#include <QUdpSocket>
#include <QDesktopServices>
#include <QHostInfo>
#include <QHostAddress>

#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include <QFile>

#define PORT 5066

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    void send_file(QString file);
private slots:
    void click_send();
    void click_file();
    void click_remove();
    void click_clean();
    void click_save_path();
    void click_udp();
    void recv_udp();
    void click_clear_udp_list();
    void tcp_socket_connect();
    void tcp_socket_recv();
    void tcp_server_recv();
    void tcp_connect();
    void time_out();
private:
    Ui::MainWindow *ui;
    QString path;
    QString save_path;
    QString filename;
    qint64 filesize;
    qint64 recvsize;
    QFile recv_file;
    QUdpSocket *udp_socket;

    QTcpServer *tcpserver;
    QTcpSocket *tcp_server_socket;
    QTcpSocket *tcpsocket;
    QTimer *timer;
    bool isStart;
    bool isOK;
};

#endif // MAINWINDOW_H
