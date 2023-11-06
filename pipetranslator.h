#ifndef PIPETRANSLATOR_H
#define PIPETRANSLATOR_H

#include <QObject>

class PipeTranslator : public QObject
{
    Q_OBJECT
public:
    PipeTranslator();
public slots:
    void StartPipeReader (char *);
    void StopPipeReader ();
signals:
    void signalMessFromPipe(QString);
private:
    QString namePipe;
    int fd,len;
};

#endif // PIPETRANSLATOR_H
