#include "mainwindow.h"
#include "sender.h"
#include <assert.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <memory.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <QThread>
#include <pthread.h>
#include <iostream>
#include <QDebug>
#include <QRandomGenerator>
#include <QDateTime>
#define NAMEDPIPE_NAME "/tmp/my_named_pipe"

using namespace std;

#define BACKLOG 512
/*
 * структура данных, передаваемых в процесс
 */
struct thread_param {
    int port;       // номер порта сервера
    int messLen;    // длина отправляемого на сервер сообщения
    int maxTimeOut; // таймаут ожидания ответа сервера
};

int stopThreads=0;  // флаг остановки процессов (1 - стоп)

extern "C"
{
/*
 * Основная функция процесса.
 * Передает сообщения на tcp-server.
 * Объявлена в секции extern "C" из-за конфликта tcp-функции connect(...) с одноименной Qt-функцией
 * param:
 *   struct thread_param*
 * return:
 *   void*
 */
void* simpleSend (void *par)
{
    int sock, ret_snd, ret_rcv;
    thread_param *param= (thread_param *)par;
    struct sockaddr_in addr;
    char buf[128];
    char buf_rcv[128];
    qint64 curTime;
    qint64 waitTime;
    int fd,ln;
    QRandomGenerator rnd;
    QDateTime start,endt;

    fd = open(NAMEDPIPE_NAME, O_WRONLY );
    if ( fd == -1 ) {
        qCritical() << Q_FUNC_INFO << "error open fifo=" << errno << "\n";
        perror("simpleSend error open fifo");
        return NULL;
    }
    memset (buf,0,sizeof(buf));
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0)
    {
        qCritical() << Q_FUNC_INFO << "error open socket=" << errno << "\n";
        perror("socket");
        exit(1);
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(param->port); // или любой другой порт...
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if(connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        qCritical() << Q_FUNC_INFO << "connect=" << errno << "\n";
        perror("connect");
        close(sock);
        exit(2);
    }

    // бесконечный цикл
    while (1) {
        // задержка на случайную величину
        rnd = QRandomGenerator::global()->generate();
        waitTime = rnd.bounded(3000000);
        usleep (waitTime);
        qDebug() << Q_FUNC_INFO << "sleep=" << waitTime << "\n";

        // генерация случайной строки длиной param->messLen
        QString characters = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
        QString randomString;

        for (int i = 0; i < param->messLen; i++) {
                int randomIndex = QRandomGenerator::global()->bounded(0, characters.length());
                QChar randomChar = characters.at(randomIndex);
                randomString.append(randomChar);
        }
        randomString.append("\n");
        qDebug() << Q_FUNC_INFO << "randomString=" << randomString << "\n";

        // отправка сообщения
        curTime = start.currentDateTime().toMSecsSinceEpoch();
        ret_snd = send(sock, randomString.toStdString().c_str(), randomString.size(), 0);
        if (ret_snd < 0) {
            qCritical() << Q_FUNC_INFO << "error send=" << errno << "\n";
            perror("send");
            close(sock);
            return NULL;
        }
        qDebug() << Q_FUNC_INFO << "sended=" << ret_snd << " bytes" << "\n";

        // ожидание прихода ответа
        ret_rcv = recv(sock, buf_rcv, sizeof(buf_rcv), 0);

        if ((ret_rcv < 0)&&(errno != EAGAIN)) {
            qCritical() << Q_FUNC_INFO << "error recv=" << errno << "\n";
            perror("recv");
            close(sock);
            return NULL;
        }
        if (ret_rcv == 0) {
            qCritical() << Q_FUNC_INFO << "Соединение разорвано сервером errno:" << errno << "\n";
            close(sock);
            return NULL;
        }
        qDebug() << Q_FUNC_INFO << "recived=" << ret_rcv << " bytes:"  << buf_rcv << "\n";

        // вычисление задержки ответа сервера
        waitTime = endt.currentDateTime().toMSecsSinceEpoch() - curTime;
        sprintf (buf,"%Ld",waitTime);
        ln = write (fd,buf,strlen(buf)+1); // пишем в pipe
        if (ln<=0) {
            qCritical() << Q_FUNC_INFO << "error write to pipe=" << errno << "\n";
            close(sock);
            return NULL;
        }
        qDebug() << Q_FUNC_INFO << "sended to pipe=" << ln << " bytes:" << buf << "\n";

        // проверяем запрос на прерывание процесса
        if (stopThreads==1) break;
    }
    shutdown (sock, SHUT_RDWR);
    close(sock);

    pthread_exit(0);
    return 0;
}

}

Sender::Sender()
{
    isRun = false;
}
/*
 * Запуск нескольких потоков simpleSend.
 * Вызывается сигналом из mainwindow signalSocketOpenViaC.
 * param:
 *   port - номер порта
 * return:
 *   void
 */
void Sender::slotSocketOpenViaC(int port)
{
    QString mess;
    struct thread_param param;

    param.port = this->port;
    param.messLen = this->messLen;

   mess.sprintf("==========\nЗапуск %d потоков по %d байт на port=%d",nThread,messLen,port);
    stopThreads = 0;
   emit  signalPrintToLog (mess);
   qDebug() << Q_FUNC_INFO << mess << "\n";
   isRun = true;

   //выделяем память под массив идентификаторов потоков
   pthread_t* threads = (pthread_t*) malloc(nThread * sizeof(pthread_t));

   for (int i=0;i<nThread;i++) {
       //запускаем поток
       pthread_create(&(threads[i]), NULL, simpleSend, &param);
   }

   while (1) {
        if (QThread::currentThread()->isInterruptionRequested() ) {
            emit  signalPrintToLog ("Ожидайте завершения потоков.....");
            qDebug() << Q_FUNC_INFO << "***Interruption requst*****\n";
            // завершение всех потоков
            stopThreads = 1;
            break;
        }
   }

   //ожидаем завершение всех потоков
   for(int i = 0; i < nThread; i++) {
       pthread_join(threads[i], NULL);
   }

   qDebug() << Q_FUNC_INFO << "Все потоки завершились.\n";
   emit  signalPrintToLog ("Все потоки завершились.");

   //освобождаем память
   free(threads);

   isRun = false;
   QThread::currentThread()->exit();
}


void Sender::slotTheadExec(int port)
{
    QThread::currentThread()->requestInterruption();
    emit  signalPrintToLog ("Stop потока.");
    //QThread::currentThread()->quit();
}
