#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpSocket>
#include <QtGui>
#include <QMessageBox>
#include <QTextCodec>
#include <QList>
#include <QBrush>
#include <QListWidgetItem>
#include <QTimer>
#include <QProcess>
#include <QSettings>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:

    friend class DialogSettings;

    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();



private slots:
    void initialapp(); // чтение настроек
    void set_address(); // запись настроек ip
    void set_port (); // запись настроек порта
    void connect_to_moxa(); // подключение к moxa для опроса
    void ask_dio_status(); // запрос состояний DIO
    void getState(); // получает информацию о состоянии
    void showState (); //размещает в окне

private:

    QSettings krn_settings; // настройки приложения

    int ask_timer; // значение таймера опроса, при ошибке подключения будет опрашиваться чаще
    QTimer *timer; // сам таймер опроса
    QList<QString> res_dio; // ответ от MOXA
    QString state_old; // Состояние КРН, с которым будем сравнивать
    QListWidgetItem *element; // Используется для установки в объекте QListWidget]
    QString ip; // адрес подключения
    quint16 port; // порт подключения
    QTcpSocket *sock; // сокет для опроса

    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
