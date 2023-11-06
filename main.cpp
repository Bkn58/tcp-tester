#include "mainwindow.h"
#include "sender.h"
#include <QApplication>
#include <QFile>
#include <QDateTime>
#include <QDebug>
#include <QTextStream>
//#include <QSettings>
#define NAME_LOG_FILE ".log"

void myMessageOutput(QtMsgType type,  const QMessageLogContext &context, const QString &msg)
{
    //QFile fMessFile(qApp->applicationDirPath() + NAME_LOG_FILE);
    QFile fMessFile(qApp->applicationFilePath() + NAME_LOG_FILE);
    if(!fMessFile.open(QFile::Append | QFile::Text)){
        return;
    }

//    QString sCurrDateTime = "[" +
//              QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss.zzz") + "]";
    QString sCurrDateTime = "[" +
              QDateTime::currentDateTime().toString("hh:mm:ss.zzz") + "]";
    QTextStream tsTextStream(&fMessFile);
    //context.line
    switch(type){
    case QtDebugMsg:
        tsTextStream << QString("%1 Debug - (%2)%3").arg(sCurrDateTime).arg(context.line).arg(msg);
        break;
    case QtWarningMsg:
        tsTextStream << QString("%1 Warning - %2").arg(sCurrDateTime).arg(msg);
        break;
    case QtCriticalMsg:
        tsTextStream << QString("%1 Critical - %2").arg(sCurrDateTime).arg(msg);
        break;
    case QtInfoMsg:
        tsTextStream << QString("%1 Info - %2").arg(sCurrDateTime).arg(msg);
    case QtFatalMsg:
        tsTextStream << QString("%1 Fatal - %2").arg(sCurrDateTime).arg(msg);
        abort();
    }

    tsTextStream.flush();
    fMessFile.flush();
    fMessFile.close();

}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;

    QFile fMessFile(qApp->applicationFilePath() + NAME_LOG_FILE);
    fMessFile.remove();
    fMessFile.close();

    qInstallMessageHandler(myMessageOutput);
    qDebug() <<  Q_FUNC_INFO << "Start program!\n";
    w.show();

    return a.exec();
}
