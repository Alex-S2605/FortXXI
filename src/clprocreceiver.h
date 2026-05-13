#ifndef CLPROCRECEIVER_H
#define CLPROCRECEIVER_H

#include <QObject>
#include <QTimer>
#include <QMutex>
#include <QVector>
#include <QDateTime>

typedef QVector <double> typeValueProc;

class clProcReceiver : public QObject
{
    Q_OBJECT
public:
    clProcReceiver();
    QMutex mutexProc;
    QVector <double> vKey;
    typeValueProc valueProc;
    double procMax;
    double procMaxV;
    QVector <typeValueProc> vValueProc;
    int countProc;

private:
    QTimer * timer;
    quint64 dtSaterted;
    quint64 counter;
    void slorReceive();

public slots:
    void slotOnStart();

signals:
    void signalProcIsReseived();

};

#endif // CLPROCRECEIVER_H
