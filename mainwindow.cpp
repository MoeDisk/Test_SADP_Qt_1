#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    tableWidget = ui->tableWidget;
    scanButton = ui->scanButton;
    executeButton = ui->executeButton;
    groupBox_2 = ui->groupBox_2;

    tableWidget->setColumnCount(2);
    QStringList headers;
    headers << "IP" << "ID";
    tableWidget->setHorizontalHeaderLabels(headers);
    tableWidget->horizontalHeader()->setStretchLastSection(true);

    connect(scanButton,&QPushButton::clicked,this,&MainWindow::discoverDevices);
    connect(executeButton,&QPushButton::clicked,this,&MainWindow::executeCommand);
    connect(tableWidget->selectionModel(), &QItemSelectionModel::currentRowChanged,
            this, &MainWindow::onRowSelected);

    udpSocket = new QUdpSocket(this);
    connect(udpSocket,&QUdpSocket::readyRead,this,&MainWindow::readResponse);
    udpSocket->bind(QHostAddress::AnyIPv4,RESPONSE_PORT,QUdpSocket::ShareAddress);

    QIcon DriveNetIcon = QApplication::style()->standardIcon(QStyle::SP_DriveNetIcon);
    ui->tabWidget->setTabIcon(0, DriveNetIcon);
    qApp->setStyleSheet(R"(
                QWidget {
                    background-color: #2f2f2f;
                    color: #e0e0e0;
                    font-family: "Segoe UI", Arial, sans-serif;
                    font-size: 14px;
                }

                QPushButton {
                    background-color: #444444;
                    color: #ffffff;
                    border-radius: 5px;
                    padding: 8px 16px;
                    border: 1px solid #666666;
                }

                QPushButton:hover {
                    background-color: #666666;
                    border: 1px solid #999999;
                }

                QPushButton:pressed {
                    background-color: #333333;
                }

                QSlider::groove:horizontal {
                    border: 1px solid #555555;
                    height: 8px;
                    border-radius: 4px;
                    background: #333333;
                }

                QSlider::handle:horizontal {
                    background: #888888;
                    border: 1px solid #666666;
                    width: 14px;
                    height: 14px;
                    margin: -3px 0;
                    border-radius: 7px;
                }

                QLabel {
                    color: #e0e0e0;
                }

                QCheckBox {
                    spacing: 5px;
                    color: #e0e0e0;
                }

                QCheckBox::indicator {
                    width: 16px;
                    height: 16px;
                }

                QCheckBox::indicator:unchecked {
                    border: 1px solid #666666;
                    background-color: #333333;
                }

                QCheckBox::indicator:checked {
                    border: 1px solid #666666;
                    background-color: #888888;
                }

                QGroupBox {
                    border: 1px solid #666666;
                    border-radius: 5px;
                    margin-top: 10px;
                    color: #e0e0e0;
                }

                QGroupBox::title {
                    subcontrol-origin: margin;
                    subcontrol-position: top center;
                    padding: 2px 5px;
                    color: #e0e0e0;
                }

                QMessageBox {
                    background-color: #444444;
                    color: #e0e0e0;
                }

                QLineEdit {
                    border: 1px solid #555555;
                    border-radius: 4px;
                    padding: 4px;
                    color: #e0e0e0;
                    selection-background-color: #888888;
                }

                QTextEdit {
                    border: 1px solid #555555;
                    border-radius: 4px;
                    padding: 4px;
                    color: #e0e0e0;
                    selection-background-color: #888888;
                }

                QComboBox {
                    border: 1px solid #555555;
                    border-radius: 4px;
                    padding: 4px;
                    color: #e0e0e0;
                    selection-background-color: #888888;
                }

                QComboBox QAbstractItemView {
                    selection-background-color: #888888;
                    color: #e0e0e0;
                }

                            QTabWidget::pane {
                                border: 1px solid #666666;
                                background: #2f2f2f;
                                border-radius: 5px;
                            }

                            QTabBar::tab {
                                background: #444444;
                                color: #e0e0e0;
                                border: 1px solid #666666;
                                padding: 6px 12px;
                                border-top-left-radius: 4px;
                                border-top-right-radius: 4px;
                            }

                            QTabBar::tab:hover {
                                background: #666666;
                                border: 1px solid #999999;
                            }

                            QTabBar::tab:selected {
                                background: #333333;
                                border: 1px solid #888888;
                                color: #ffffff;
                            }
                            QTableWidget {
                                border: 1px solid #555555;
                                border-radius: 4px;
                                gridline-color: #666666;
                                background-color: #2f2f2f;
                                color: #e0e0e0;
                                selection-background-color: #666666;
                                selection-color: #ffffff;
                            }

                            QTableWidget::item {
                                border: none;
                                padding: 4px;
                            }

                            QTableWidget::item:selected {
                                background-color: #444444;
                                color: #ffffff;
                            }

                            QHeaderView::section {
                                background-color: #444444;
                                padding: 4px;
                                border: 1px solid #666666;
                                color: #e0e0e0;
                            }

                            QHeaderView::section:horizontal {
                                border-top-left-radius: 4px;
                                border-top-right-radius: 4px;
                            }

                            QHeaderView::section:vertical {
                                border-left: none;
                                border-right: none;
                            }
            )");
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::discoverDevices(){
    tableWidget->setRowCount(0);
    QByteArray data = "from -> Test_SADP_Qt_1";
    udpSocket->writeDatagram(data,QHostAddress(MULTICAST_ADDR),MULTICAST_PORT);
}

void MainWindow::onRowSelected(const QModelIndex &current, const QModelIndex &previous) {
    int row = current.row();
    if (row >= 0) {
        QString randomID = tableWidget->item(row, 1)->text();
        groupBox_2->setTitle(QString("%1").arg(randomID));
    }
}

void MainWindow::readResponse() {
    while (udpSocket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(udpSocket->pendingDatagramSize());
        QHostAddress sender;
        quint16 senderPort;
        udpSocket->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);

        QString response = QString(datagram);
        qDebug() << "Received response: " << response;

        QStringList parts = response.split(',');

        if (parts.size() == 2) {
            QString deviceIP = parts[0];
            QString randomID = parts[1];
            qDebug() << "Device IP: " << deviceIP;
            qDebug() << "Random ID: " << randomID;

            bool idExists = false;
            for (int row = 0; row < tableWidget->rowCount(); ++row) {
                if (tableWidget->item(row, 1)->text() == randomID) {
                    idExists = true;
                    break;
                }
            }

            if (!idExists) {
                int row = tableWidget->rowCount();
                tableWidget->insertRow(row);
                tableWidget->setItem(row, 0, new QTableWidgetItem(deviceIP));
                tableWidget->setItem(row, 1, new QTableWidgetItem(randomID));
                int totalRows = tableWidget->rowCount();
                setWindowTitle(QString("Qt的UDP组播测试 - %1 Devices").arg(totalRows));
            } else {
                qDebug() << "Duplicate random ID, not adding to the table.";
            }
        } else {
            qDebug() << "Invalid response format";
        }
    }
}


void MainWindow::executeCommand() {
    int currentRow = tableWidget->currentRow();
    if (currentRow == -1) {
        QMessageBox::warning(this, "No Selection", "请先在列表选择一个IP地址。");
        return;
    }

    QString selectedIP = tableWidget->item(currentRow, 0)->text();
    QString randomId = tableWidget->item(currentRow, 1)->text();

    if (selectedIP.isEmpty() || randomId.isEmpty()) {
        QMessageBox::warning(this, "Invalid Data", "选中的设备信息不完整！");
        return;
    }

    bool ok;
    QString newIP = QInputDialog::getText(this, "Enter IP Address", "请输入新的设备IPv4地址：", QLineEdit::Normal, "", &ok);
    if (!ok || newIP.isEmpty()) {
        QMessageBox::warning(this, "Invalid Input", "IP地址不能为空！");
        return;
    }

    QString subnetMask = QInputDialog::getText(this, "Enter Subnet Mask", "请输入子网掩码：", QLineEdit::Normal, "", &ok);
    if (!ok || subnetMask.isEmpty()) {
        QMessageBox::warning(this, "Invalid Input", "子网掩码不能为空！");
        return;
    }

    QString gateway = QInputDialog::getText(this, "Enter Gateway", "请输入网关地址：", QLineEdit::Normal, "", &ok);
    if (!ok || gateway.isEmpty()) {
        QMessageBox::warning(this, "Invalid Input", "网关地址不能为空！");
        return;
    }

    QByteArray command = "SET_IP," + randomId.toUtf8() + "," + newIP.toUtf8() + "," + subnetMask.toUtf8() + "," + gateway.toUtf8();

    udpSocket->writeDatagram(command, QHostAddress(MULTICAST_ADDR), CONTROL_PORT);

    QMessageBox::information(this, "Command Sent", "执行命令已发送到设备 " + selectedIP);
}
