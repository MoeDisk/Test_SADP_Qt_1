#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#pragma execution_character_set("UTF-8")

#include <QMainWindow>
#include <QUdpSocket>
#include <QTableWidget>
#include <QPushButton>
#include <QGroupBox>
#include <QMessageBox>
#include <QInputDialog>
#include <QStringList>
#include <QHostAddress>

#define MULTICAST_ADDR "224.0.0.1"
#define MULTICAST_PORT 6000
#define RESPONSE_PORT 6001
#define CONTROL_PORT 6002

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    QPushButton *scanButton;
    QTableWidget *tableWidget;
    QPushButton *executeButton;
    QGroupBox *groupBox_2;
    QUdpSocket *udpSocket;

private slots:
    void discoverDevices();
    void readResponse();
    void executeCommand();
    void onRowSelected(const QModelIndex &current, const QModelIndex &previous);
};
#endif // MAINWINDOW_H
