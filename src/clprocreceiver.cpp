#include "clprocreceiver.h"
#include "math.h"

#include <QDebug>

clProcReceiver::clProcReceiver()
    : countProc(0)
    , counter(-1)
{}

void clProcReceiver::slorReceive()
{
    qDebug() << "clProcReceiver::slorReceive vP.size begin" << countProc;
    if (countProc == 0) {
        countProc = 4;
        vKey.reserve(300);
        valueProc.reserve(300);
        vValueProc.resize(countProc);
        for(int i = 0; i < countProc; i++) vValueProc[i].reserve(300);
        foreach(typeValueProc vP, vValueProc) qDebug() << "clProcReceiver::slorReceive vVP.size" << vP.capacity();
        dtSaterted = QDateTime::currentDateTime().currentSecsSinceEpoch();
    }
    counter++;
    mutexProc.lock();
    quint64 cTime = dtSaterted + counter;
    double a, o;
    double value = 0, val;
    procMaxV = 0;
    for(int i = 0; i < countProc; i++) {
        a = 30 / (i + 1);
        o = M_PI * 2. / countProc * i;
        if (vValueProc[i].size() == 300) vValueProc[i].takeAt(0);
        val = a * (1 + sin(cTime * M_PI * 2. / 300 + o));
        vValueProc[i] << val;
        value += val;
        auto maxElement = std::max_element(vValueProc[i].begin(), vValueProc[i].end());
        if (procMaxV < *maxElement) procMaxV = *maxElement;
    }
    valueProc << value;

    auto maxElement = std::max_element(valueProc.begin(), valueProc.end());

    procMax = *maxElement;
    if (vKey.size() == 300) vKey.takeAt(0);
    vKey << cTime;
    qDebug() << "clProcReceiver::slorReceive vP.size vKey valueProc" << vKey.size() << valueProc.size() << procMax;
    emit signalProcIsReseived();
    mutexProc.unlock();
}

void clProcReceiver::slotOnStart()
{
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &clProcReceiver::slorReceive, Qt::QueuedConnection);
    timer->start(1000);
}
