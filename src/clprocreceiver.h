#ifndef CLPROCRECEIVER_H
#define CLPROCRECEIVER_H

#include <QObject>
#include <QTimer>
#include <QMutex>
#include <QVector>
#include <QDateTime>
#include <QUdpSocket>

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
    QUdpSocket * socket;
    bool bFirstData;
    void slotReceive();
    quint32 messageCounter;

public slots:
    void slotOnStart();

private slots:
    void slotReadSocket();

signals:
    void signalProcIsReseived();

};

#endif // CLPROCRECEIVER_H
