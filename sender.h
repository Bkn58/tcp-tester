#ifndef SOCKET_H
#define SOCKET_H
#include <QWidget>
//#include <QMainWindow>
#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include<QThread>

namespace Ui  {
class Sender;
}
class Sender : public QObject //QThread
{
  Q_OBJECT
public:
    Sender();
    int  SocketOpenViaC (int);
    int  SimpleSendViaC (int);
    void* SendViaCInCicle (void*);
    void* SendOnceViaC (void *);
    void SendVariosPacket (int);
    bool IsExecute() {
        return isRun;
    }
    void setPort (int prt) {port = prt;}
    void setMessLen (int len) {messLen = len;}
    void setNumThread (int num) {nThread = num;}
public slots:
    void slotSocketOpenViaC(int port);
    void slotTheadExec (int port);
signals:
    void signalPrintToLog(QString);
private:
    bool isRun;
    int port;
    int messLen;
    int nThread;
};
#endif // SOCKET_H
