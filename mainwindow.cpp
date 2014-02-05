#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <qtcpsocket.h>
#include <QString>
#include <QLineEdit>


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    krn_settings("OOE", "krn_narishkino"),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);


    QTextCodec* codec = QTextCodec::codecForName("UTF8"); // указываем кодировку
    QTextCodec::setCodecForTr(codec);
    QTextCodec::setCodecForCStrings(codec);
    QTextCodec::setCodecForLocale(codec);


    this->move(QApplication::desktop()->availableGeometry().topLeft()); // запуск окна в левом верхнем углу
    //создаем сокет
    sock = new QTcpSocket(this); // сокет для опроса

    ui->ip_label->setText(ip);
    //соединяем сигналы и прочее
    state_old=""; // обнулили состояние
    ask_timer=60000;
    timer = new QTimer; // таймер по которому будет копироваться файл и соответственно запускаться его сравнение
    connect(timer, SIGNAL(timeout()), this, SLOT(connect_to_moxa())); // соединение сигнала таймера и считывания DIO
    connect(ui->Refresh_but, SIGNAL(clicked()), this, SLOT(connect_to_moxa())); // Обновление по нажатию кнопки
    connect(sock, SIGNAL(connected()), this, SLOT(ask_dio_status()));
    connect(sock, SIGNAL(readyRead()), this, SLOT(getState()));
    connect(ui->Address, SIGNAL(triggered()), this, SLOT(set_address()));
    connect(ui->Port, SIGNAL(triggered()), this, SLOT(~MainWindow()));

    initialapp();
}


/********************** Инициализация приложения *************************/
void MainWindow::initialapp()
{
    if ((!krn_settings.value("/settings/address").isNull()) && (!krn_settings.value("/settings/port").isNull()))
    {
        ip = krn_settings.value("/settings/address").toString();
        port = krn_settings.value("/settings/port").toString().toShort();
        ui->ip_label->setText(ip);
        connect_to_moxa();
    }
    else
    {
        ip = "Нет настроек подключения!";
        ui->ip_label->setText(ip);
    }
}

/********************* Запись настроек IP *********************/
void MainWindow::set_address()
{
    bool set_ip;
    QString str = QInputDialog::getText(this, "Введите адрес", "Адрес:", QLineEdit::Normal, "", &set_ip);

    if (!set_ip)
    {
        return;
    }
    else
    {
        krn_settings.setValue("/settings/address", str);
        initialapp();
    }
}

/**************** Запись настроек порта *******************/
void MainWindow::set_port()
{
    bool set_port;
    QString str = QInputDialog::getText(this, "Введите порт", "Порт:", QLineEdit::Normal, "", &set_port);

    if (!set_port)
    {
        return;
    }
    else
    {
        krn_settings.setValue("/settings/port", str);
        initialapp();
    }
}


/************************************** Подключение к мохе *******************************/
void MainWindow::connect_to_moxa()
{
    timer->start(ask_timer); //запуск таймера
    sock->connectToHost(ip, port); // Соединяемся с указанным адресом
    if (!sock->waitForConnected(10000)) // ели за 10 секунд сервер нас не пустит, то выдаем ошибку.
    {
        ask_timer=15000; // меняем частоту опроса

        QTextCodec* codec = QTextCodec::codecForName("UTF8"); // указываем кодировку
        QTextCodec::setCodecForTr(codec);
        QTextCodec::setCodecForCStrings(codec);
        QTextCodec::setCodecForLocale(codec);
        QBrush blue = QBrush(QColor(100, 100, 255), Qt::SolidPattern); // Кисть синего цвета

        ui->Statewgt -> clear(); // Очистка области под флажки.
        ui->Statewgt -> setResizeMode(QListView::Adjust);
        ui->Statewgt -> setMovement(QListView::Static); // Указываем, что элементы не могут быть перемещены пользователем.
        ui->Statewgt -> setSpacing(8);
        ui->Statewgt -> setFlow(QListView::LeftToRight); // Устанавливаем порядок расположения элементов слева на право.
        ui->Statewgt -> setWrapping(true); // Устанавливаем свойство, при  котором элементы переносятся, если выходят за область отображения.

        element = new QListWidgetItem("", ui->Statewgt); // Создаем новый экземпляр для элемента объекта QListWidget, указывая этот объект аргументом в качестве родителя.
        element -> setSizeHint(QSize(95,24));

        element->setBackground(blue);
        element -> setText("Нет связи!");

        qDebug() << "Нет связи!";

        if (state_old!="err")
        {
        QProcess beep;
        QString bep = "beep -f 2000 -n -f 1500 -n -f 1000";
        beep.start(bep);
        beep.waitForFinished(1000);
        state_old="err";
        }
    }
}


/******************************** ОТПРАВКА ОПРОСА MOXA *******************************/
void MainWindow::ask_dio_status ()
{
    ask_timer=60000; // возвращаем частоту опроса после обрыва связи

    QByteArray ask_dio; // команда запроса состояний

    ask_dio.resize(6); // записываем в массив команду. Описание команды см в инструкции к MOXA 4100
    ask_dio[0]=5;
    ask_dio[1]=2;
    ask_dio[2]=0;
    ask_dio[3]=2;
    ask_dio[4]=0;
    ask_dio[5]=3;

    sock->write(ask_dio); // отправка запроса
}


/**************************** ПОЛУЧЕНИЕ ОТВЕТА **************************/
void MainWindow::getState()
{
    res_dio.clear(); // очищаем результат ответа

    int i=sock->bytesAvailable(); // количество байт, которое будет принято
    int j; // счетчик
    for (j=0; j<i; j++)
    {
        res_dio << sock->read(1).toHex(); // Пишем в список побайтно ответ от MOXA
    }
    sock->close(); // закрываем соединение

    qDebug() << "Multiple DIO Status: " << res_dio;

    // если состояние изменилось, то рисуем это в окне
    if (res_dio.at(5)!=state_old)
    {
    showState (); // Запускаем формирование окна состояний
    }
}


/******************** ФОРМИРОВАНИЕ ОКНА СОСТОЯНИЙ *******************/
void MainWindow::showState ()
{
    QTextCodec* codec = QTextCodec::codecForName("UTF8"); // указываем кодировку
    QTextCodec::setCodecForTr(codec);
    QTextCodec::setCodecForCStrings(codec);
    QTextCodec::setCodecForLocale(codec);
    QBrush green = QBrush(QColor(100, 255, 100), Qt::SolidPattern); // Кисть зеленого цвета
    QBrush red = QBrush(QColor(255, 100, 100), Qt::SolidPattern); // Кисть красного цвета

    //ui->ip_label->setText(ip);
    ui->Statewgt -> clear(); // Очистка области под флажки.
    ui->Statewgt -> setResizeMode(QListView::Adjust);
    ui->Statewgt -> setMovement(QListView::Static); // Указываем, что элементы не могут быть перемещены пользователем.
    ui->Statewgt -> setSpacing(8);
    ui->Statewgt -> setFlow(QListView::LeftToRight); // Устанавливаем порядок расположения элементов слева на право.
    ui->Statewgt -> setWrapping(true); // Устанавливаем свойство, при  котором элементы переносятся, если выходят за область отображения.


        element = new QListWidgetItem("", ui->Statewgt); // Создаем новый экземпляр для элемента объекта QListWidget, указывая этот объект аргументом в качестве родителя.
        element -> setSizeHint(QSize(95,24));

        if (res_dio.at(5)=="00")
        {
            element->setBackground(green);
            element -> setText("Включен");
            state_old=res_dio.at(5);
        }
        else
        {
            element->setBackground(red);
            element -> setText("Выключен");
            state_old=res_dio.at(5);
        }

        QProcess beep;
        QString bep = "beep -f 2000 -n -f 1500 -n -f 1000";
        beep.start(bep);
        beep.waitForFinished(1000);
}

MainWindow::~MainWindow()
{
    sock->close();
    delete ui;
}
