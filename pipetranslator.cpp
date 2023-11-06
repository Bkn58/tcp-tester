#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <QThread>
#include <QDebug>
#include "pipetranslator.h"

#define NAMEDPIPE_NAME "/tmp/my_named_pipe"
#define BUFSIZE        100

PipeTranslator::PipeTranslator()
{

}

void PipeTranslator::StartPipeReader(char *name)
{
    char buf[BUFSIZE];
    namePipe = name;
    QString message;

    // создаем named pipe
    int ret = mkfifo(NAMEDPIPE_NAME, 0666); //0777
    if (ret != 0) {
        qDebug() << Q_FUNC_INFO << "mkfifo error" << errno << "\n";

    }
    fd = open(name, O_RDONLY );  //| O_NONBLOCK)
    if ( (fd <= 0 )) {
        qCritical() << Q_FUNC_INFO << "open pipe error" << errno << "\n";
        return;
    }
    message.sprintf("%s is opened for read\n", name);
    qDebug() << Q_FUNC_INFO << message << "\n";
    // читаем pipe
    do {
        memset(buf, '\0', BUFSIZE);
        len = read(fd, buf, BUFSIZE-1);
        if ( len > 0 ) {
            message.sprintf("%s\n", buf);
            emit signalMessFromPipe (message);
            qDebug() << Q_FUNC_INFO << message << "\n";
        }
    } while ( 1 );

}

void PipeTranslator::StopPipeReader()
{
    close(fd);
}
