#include "clprocreceiver.h"
#include "math.h"

#include <QDebug>

clProcReceiver::clProcReceiver()
    : countProc(0)
    , counter(-1)
    , bFirstData(true)
{}

void clProcReceiver::slotReceive() // Функция для автономной отладки графиков
{
    qDebug() << "clProcReceiver::slotReceive vP.size begin" << countProc;
    if (countProc == 0) {
        countProc = 4;
        vKey.reserve(300);
        valueProc.reserve(300);
        vValueProc.resize(countProc);
        for(int i = 0; i < countProc; i++) vValueProc[i].reserve(300);
        foreach(typeValueProc vP, vValueProc) qDebug() << "clProcReceiver::slotReceive vVP.size" << vP.capacity();
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
    qDebug() << "clProcReceiver::slotReceive vP.size vKey valueProc" << vKey.size() << valueProc.size() << procMax;
    emit signalProcIsReseived();
    mutexProc.unlock();
}

void clProcReceiver::slotOnStart()
{
    qDebug() << "clProcReceiver::slotOnStart begin";
    // timer = new QTimer(this);
    // connect(timer, &QTimer::timeout, this, &clProcReceiver::slotReceive, Qt::QueuedConnection);
    // timer->start(1000);
    socket = new QUdpSocket(this);
    if (socket) {
        qDebug() << "clProcReceiver::slotOnStart socket";
        // socket->disconnectFromHost();
        bool res = socket->bind(QHostAddress::Any, 1234);
        // bool res = socket->bind(QHostAddress::AnyIPv4, 1234, QAbstractSocket::ShareAddress);
        qDebug() << Q_FUNC_INFO << "res" << res << socket->BoundState;
        if (socket->hasPendingDatagrams()) {
            qDebug() << "clProcReceiver::slotOnStart hasPendingDatagrams";
            QByteArray dg = socket->readAll();
            qDebug() << "clProcReceiver::slotOnStart Datagram" << dg;
        }
        connect(socket, &QUdpSocket::readyRead, this, &clProcReceiver::slotReadSocket);
    }
    qDebug() << "clProcReceiver::slotOnStart end";
}

void clProcReceiver::slotReadSocket()
{
    qDebug() << "clProcReceiver::slotReadSocket begin";
    if (socket->state() != QAbstractSocket::BoundState) {
        qDebug() << " lProcReceiver::slotReadSocketSocket not bound in slotReadSocket, state:" << socket->state();
        return;
    }
    if (socket->hasPendingDatagrams()) {
        QByteArray dg;
        dg.resize(socket->pendingDatagramSize());
        qint64 len = socket->readDatagram(dg.data(), dg.size());
        qDebug() << "clProcReceiver::slotReadSocket Datagram" << len << dg;
        QDataStream s(dg);
        s.setByteOrder(QDataStream::LittleEndian);
        s.setFloatingPointPrecision(QDataStream::SinglePrecision);
        QByteArray bytes = s.device()->read(5);
        qDebug() << "clProcReceiver::slotReadSocket bytes" << bytes;
        if (bytes != "BEGIN") {
            qDebug() << "clProcReceiver::slotReadSocket return on BEGIN";
            return;
        }
        quint16 cpuCountIn;
        s >> messageCounter >> cpuCountIn;
        qDebug() << "clProcReceiver::slotReadSocket messageCounter cpuCount" << messageCounter << cpuCountIn;
        if (bFirstData) {
            bFirstData = false;
            countProc = cpuCountIn;
            vKey.reserve(300);
            valueProc.reserve(300);
            vValueProc.resize(countProc);
            for(int i = 0; i < countProc; i++) vValueProc[i].reserve(300);
            foreach(typeValueProc vP, vValueProc) qDebug() << "clProcReceiver::slotReadSocket vVP.size" << vP.capacity();
            dtSaterted = QDateTime::currentDateTime().currentSecsSinceEpoch();
        }
        float value = 0;
        bool valid;
        s >> valid;
        if (valid) s >> value;
        else s.skipRawData(4);
        mutexProc.lock();
        if (valueProc.size() == 300) valueProc.takeAt(0);
        valueProc << value;
        qDebug() << "clProcReceiver::slotReadSocket value" << value;
        auto maxElement = std::max_element(valueProc.begin(), valueProc.end());
        procMax = *maxElement;
        for(int i = 0; i < countProc; i++) {
            s >> valid;
            if (valid) s >> value;
            else s.skipRawData(4);
            if (vValueProc[i].size() == 300) vValueProc[i].takeAt(0);
            vValueProc[i] << value;
            qDebug() << "clProcReceiver::slotReadSocket i value" << i << valid << value;
            auto maxElement = std::max_element(vValueProc[i].begin(), vValueProc[i].end());
            if (procMaxV < *maxElement) procMaxV = *maxElement;
        }
        quint64 cTime = dtSaterted + counter;
        if (vKey.size() == 300) vKey.takeAt(0);
        vKey << cTime;
        qDebug() << "clProcReceiver::slotReadSocket vP.size vKey valueProc" << vKey.size() << valueProc.size() << procMax;
        emit signalProcIsReseived();
        mutexProc.unlock();


        counter++;
    }
}

