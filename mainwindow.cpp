#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <chrono>
#include <thread>
#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QThread>
#include <iostream>
#include <QDebug>
#include <QGraphicsScene>
#include <QProgressDialog>

#define NAMEDPIPE_NAME "/tmp/my_named_pipe"
#define BUFSIZE        50
extern "C"
{
int close(int fd);
int closeFd (int fd) {
    return close (fd);
}
}
using namespace std;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    QString message;
    // чтение/запись настроек в файл  .conf (в каталоге запуска приложения)
    conf = new QSettings(qApp->applicationFilePath() + INI_FILE,QSettings::IniFormat);

    ui->setupUi(this);
    ui->statusbar->addWidget(ui->lbTemp);
    ui->statusbar->addWidget(ui->lbPort);
    ui->statusbar->addWidget(ui->lbMessStat);
    ui->statusbar->addWidget(ui->progressBar);
    ui->progressBar->hide();
    ui->txtLog->append("Press Start to begin.");
    ui->txtLog->show();

    message.sprintf("Threads=%d",ui->sldTemp->value());
    ui->lbTemp->setText(message);
    ui->lcdThread->display(ui->sldTemp->value());
    ui->lcdLenMess->display(ui->sldMessLen->value());

    // чтение config
    QString confPort = conf->value("port").toString();
    QString confTimeout = conf->value("timeout").toString();
    QString confMinThread = conf->value("minthread").toString();
    QString confMaxThread = conf->value("maxthread").toString();
    QString confMaxMess = conf->value("maxmess").toString();

    // default options
    if (confPort=="") {
        ui->portNum->append("8080");
        conf->setValue("port",port);
    }
    else
        ui->portNum->append(confPort);
    if(confTimeout=="") {
        ui->textTimeout->append("3000");
        conf->setValue("timeout",maxWaitTime);
    }
    else
        ui->textTimeout->append(confTimeout);
    if (confMinThread == "") {
        ui->textMinThread->append("1");
        conf->setValue("minthread",ui->textMinThread->toPlainText().toInt());
    }
    else
        ui->textMinThread->append(confMinThread);
    if (confMaxThread == "") {
        ui->textMaxThread->append("100");
        conf->setValue("maxthread",ui->textMaxThread->toPlainText().toUInt());
    }
    else
        ui->textMaxThread->append(confMaxThread);
    if (confMaxMess == "") {
        ui->textMaxLenMess->append("128");
        conf->setValue("maxmess",ui->textMaxLenMess->toPlainText().toInt());
    }
    else
        ui->textMaxLenMess->append(confMaxMess);

    conf->sync(); // сохранение настроек

    message.sprintf("Port=");
    message.append(ui->portNum->toPlainText());
    ui->lbPort->setText(message);

    port = ui->portNum->toPlainText().toInt();
    messLen = ui->lcdLenMess->value();
    nThread = ui->lcdThread->value();
    message.sprintf("Port=%d, MessLen=%d, nThread=%d\n", port,messLen,nThread);
    ui->txtLog->append(message);

    maxWaitTime = ui->textTimeout->toPlainText().toInt();
    ui->sldTemp->setMinimum(confMinThread.toInt());
    ui->sldTemp->setMaximum(confMaxThread.toUInt());
    ui->sldMessLen->setMaximum(confMaxMess.toInt());

    soc   = new Sender();
    trans = new PipeTranslator();

    soc->setPort(port);
    soc->setMessLen(messLen);
    soc->setNumThread(nThread);

   connect (this,&MainWindow::signalSocketOpenViaC,soc,&Sender::slotSocketOpenViaC);
   connect (this,&MainWindow::signalThreadExec, soc, &Sender::slotTheadExec);
   connect (this,&MainWindow::signalPipeReaderStart,trans,&PipeTranslator::StartPipeReader);
   connect (this,&MainWindow::signalPipeReaderStop, trans, &PipeTranslator::StopPipeReader);
   connect (soc,&Sender::signalPrintToLog,this,&MainWindow::printToLog);
   connect (trans,&PipeTranslator::signalMessFromPipe,this,&MainWindow::reciveFromPipe);

   // создаем named pipe
   int ret = mkfifo(NAMEDPIPE_NAME, 0666);
   if (ret!=0) {
       perror ("MainWindow::on_pbStart_clicked mkfifo");
   }
   // помещаем в отдельные потоки
   trans->moveToThread(&pipeReaderThread);
   pipeReaderThread.start();
   emit signalPipeReaderStart((char*)NAMEDPIPE_NAME);

   message.sprintf("%s is created\n", NAMEDPIPE_NAME);
   ui->txtLog->append(message);

   penGreen = new QPen(Qt::green); // Задаём зеленую кисть
   penBlack = new QPen(Qt::black); // Задаём чёрную кисть
   penRed   = new QPen (Qt::red);  // Задаём красную кисть
   scene = new QGraphicsScene ();
   ui->grMonitor->setScene(scene);
   ui->grMonitor->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff); // Отключим скроллбар по горизонтали
   ui->grMonitor->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);   // Отключим скроллбар по вертикали
   ui->grMonitor->setAlignment(Qt::AlignCenter);                        // Делаем привязку содержимого к центру
   ui->grMonitor->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);    // Растягиваем содержимое по виджету

   width = ui->grMonitor->width();      // определяем ширину нашего виджета
   height = ui->grMonitor->height();    // определяем высоту нашего виджета
   cur_x = 0;
   scale = maxWaitTime / (height/2);        // количество милисекунд на один пиксел

   scene ->addLine(0,height/2,width,height/2,*penBlack); // чертим среднюю линию
   scene ->addLine(width,0,width,height,*penBlack); // чертим крайнюю линию

   ui->pbStop->setDisabled(true);

}

MainWindow::~MainWindow()
{
    delete ui;
    delete soc;
    delete trans;
    workerThread.quit();
    workerThread.wait();
    remove(NAMEDPIPE_NAME);
    pipeReaderThread.quit();
    pipeReaderThread.wait();


}
/*
 * Слот вывода сообщения text на информационную панель txtLog.
 */
void MainWindow::printToLog (QString text) {
    ui->txtLog->append(text);
    qApp->processEvents();
}
/*
 * Слот прием сообщений из pipe.
 * Отрисовка графика.
 */
void MainWindow::reciveFromPipe(QString str)
{
    int val = str.toInt();
    int h_val; // высота линии
    int delta;
    QString mess;

    qDebug() << Q_FUNC_INFO << "получено из pipe:" << str << "\n";
    ui->grMonitor->setAlignment(Qt::AlignCenter);                        // Делаем привязку содержимого к центру
    ui->grMonitor->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);    // Растягиваем содержимое по виджету

    width = ui->grMonitor->width();      // определяем ширину нашего виджета
    height = ui->grMonitor->height();    // определяем высоту нашего виджета

    if (cur_x >= (width-1)) {
        // достигнута граница графика - необходимо смещение влево
        scene->clear();
        cur_x = 0;
    }

    scale = maxWaitTime / (height/2);    // количество милисекунд на один пиксел

    h_val = val / scale;                 // вычисляем высоту рисуемой линии

    qDebug() << Q_FUNC_INFO << "width =" << width << "height =" << height << "scale =" << scale <<"\n";
    qDebug() << Q_FUNC_INFO << "высота линии =" << h_val << "\n";
    if (val <= maxWaitTime) {
        if (val == 0) {
            faultMess++;
            scene ->addLine(cur_x,0,cur_x,height,*penBlack);   // чертим черную линию
        }
        else {
            successMess++;
            scene ->addLine(cur_x,height,cur_x,height-h_val,*penGreen);   // чертим зеленую линию
        }
    }
    else {
        faultMess++;
        delta = val - maxWaitTime;
        scene ->addLine(cur_x,height,cur_x,height-h_val-delta,*penRed);     // чертим красную линию
    }
    scene ->addLine(0,height/2,width,height/2,*penBlack); // чертим среднюю линию
    cur_x++;
    mess.sprintf("success:%d/fault:%d",successMess,faultMess);
    ui->lbMessStat->setText(mess);
}

void MainWindow::on_sldTemp_sliderMoved(int position)
{
    QString message;

    message.sprintf("Threads=%d",position);
    ui->sldTemp->setValue(position);
    ui->lbTemp->setText(message);
    ui->lbTemp->setVisible(true);

    nThread = position;
    ui->lcdThread->display(nThread);
    soc->setNumThread(nThread);
}

void MainWindow::on_pbExit_clicked()
{
    exit (0);
}

void MainWindow::on_sldTemp_valueChanged(int value)
{
    QString message;

    message.sprintf("Temp=%d",value);
    ui->sldTemp->setValue(value);
    ui->lbTemp->setText(message);
    ui->lbTemp->setVisible(true);

    nThread = value;
    ui->lcdThread->display(nThread);
    soc->setNumThread(nThread);
}
/*
 * Запуск процессов.
 */
void MainWindow::on_pbStart_clicked()
{
     QString message;

     if (soc->IsExecute()) {
        qDebug() << Q_FUNC_INFO << "  soc is running " << "\n";
        return;
    }
     ui->pbStart->setDisabled(true);
    // создаем поток для генераторов сообщений на tcp-server
    soc->moveToThread(&workerThread);
    workerThread.start();
    successMess = 0;
    faultMess = 0;

    // запуск генераторов сообщений
    qDebug() << Q_FUNC_INFO << "========== Start ==========" << "\n";
    emit signalSocketOpenViaC (ui->portNum->toPlainText().toInt());
    ui->pbStop->setEnabled(true);
}
/*
 * Остановка процессов путем вызова requestInterruption()
 */
void MainWindow::on_pbStop_clicked()
{

    ui->pbStop->setDisabled(true);
    ui->pbStart->setDisabled(true);

    // посылаем сигнал остановки потока
    ui->txtLog->append("Request interruption begin. WAIT.....");
    qApp->processEvents();

    workerThread.requestInterruption();

    // строим прогресс бар в статус лайн для ожидания завершения workerThread
    // искусственно завышаем количество потоков, чтобы не выйти раньше времени
    int msecProThread = ((maxWaitTime+(maxWaitTime/2))/(nThread))*1000; // микросекунд на поток
    ui->progressBar->setValue(0);
    ui->progressBar->setMaximum(nThread*2);
    ui->progressBar->show();
    for (int i = 0; i < nThread*2; i++)
    {
      ui->progressBar->setValue(i) ;
      qApp->processEvents();
      usleep(msecProThread);
      // проверяем реальное окончание workerThread
      if (!workerThread.isRunning())
          break;
    }
    ui->progressBar->setValue(nThread) ;
    ui->progressBar->hide();

    workerThread.quit();
    workerThread.wait();


    QString mess;
    mess.sprintf("Итого success:%d fault:%d",successMess,faultMess);
    emit printToLog(mess);

    qDebug() << Q_FUNC_INFO << "========== Stop ==========" << "\n";

    ui->pbStart->setEnabled(true);

}

void MainWindow::on_portNum_textChanged()
{
    QString message;

    message.sprintf("Port=");
    message.append(ui->portNum->toPlainText());
    ui->lbPort->setText(message);
}


void MainWindow::on_sldMessLen_valueChanged(int value)
{
    messLen = value;
    ui->lcdLenMess->display(messLen);
    soc->setMessLen(messLen);
}

void MainWindow::on_pbSaveOptions_clicked()
{
    port = ui->portNum->toPlainText().toInt();
    maxWaitTime = ui->textTimeout->toPlainText().toInt();
    ui->sldTemp->setMinimum(ui->textMinThread->toPlainText().toInt());
    ui->sldTemp->setMaximum(ui->textMaxThread->toPlainText().toUInt());
    ui->sldMessLen->setMaximum(ui->textMaxLenMess->toPlainText().toInt());
    // запись в config
    conf->setValue("port",port);
    conf->setValue("timeout",maxWaitTime);
    conf->setValue("minthread",ui->textMinThread->toPlainText().toInt());
    conf->setValue("maxthread",ui->textMaxThread->toPlainText().toUInt());
    conf->setValue("maxmess",ui->textMaxLenMess->toPlainText().toInt());
    conf->sync();

}
