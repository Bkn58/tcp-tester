#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "sender.h"
#include "pipetranslator.h"
#include <QThread>
#include <QSettings>
#include <QGraphicsScene>
#include <QPen>

//#define INI_FILE      "config.ini" // /home/bkn/.config/Unknown Organization/ex1.conf
#define INI_FILE      ".conf" // /home/bkn/.config/Unknown Organization/ex1.conf

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT
    QThread workerThread;
    QThread pipeReaderThread;
    QThread waitDialogThread;

public:
    int port;
    int messLen;
    int nThread;
    int maxWaitTime; // максимальное время ожидания ответа
    QSettings *conf;

    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void printToLog (QString str);
    void reciveFromPipe (QString str);

private slots:
    void on_sldTemp_sliderMoved(int position);

    void on_pbExit_clicked();

    void on_sldTemp_valueChanged(int value);

    void on_pbStart_clicked();

    void on_pbStop_clicked();

    void on_portNum_textChanged();

    void on_sldMessLen_valueChanged(int value);

    void on_pbSaveOptions_clicked();

private:
    Ui::MainWindow *ui;
    Sender *soc;
    PipeTranslator *trans;
    QGraphicsScene *scene;
    QPen *penGreen; // Задаём зеленую кисть
    QPen *penBlack; // Задаём чёрную кисть
    QPen *penRed;   // Задаём красную кисть
    int cur_x;      // текущая координата времени
    int scale;      // масштаб (точек/милисек)
    int width;      // ширина нашего виджета
    int height;     // высота нашего виджета
    int successMess;// количество успешных ответов (полученных в отведенный интервал ожидания)
    int faultMess;  // количество пакетов, задержанных дольше отведенного интервала ожидания
    int fd,len;

signals:
    void  signalSocketOpenViaC (int port);
    void  signalThreadExec(int port);
    void  signalPipeReaderStart (char *);
    void  signalPipeReaderStop ();
};
#endif // MAINWINDOW_H
