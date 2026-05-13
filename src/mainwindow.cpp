#include "mainwindow.h"

#include <QVBoxLayout>
#include <QGroupBox>
#include <QPalette>

const QVector<QColor> colorV = {QColor(0, 0, 0), QColor(0, 0, 255), QColor(0, 255, 0), QColor(255, 0, 0),
    QColor(255, 255, 0), QColor(0, 255, 255), QColor(255, 0, 255), QColor(0, 0, 255),
    QColor(0, 128, 255), QColor(0, 255, 128), QColor(128, 0, 255), QColor(255, 0, 128),
    QColor(255, 128, 0), QColor(128, 255, 0), QColor(0, 0, 128), QColor(0, 128, 0),
    QColor(128, 0, 0), QColor(128, 128, 0), QColor(128, 0, 255), QColor(0, 128, 128),
    QColor(128, 128, 128)};

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , cpProcStat(new QCustomPlot)
    , procReceiver(new clProcReceiver)
    , procThread(new QThread(this))
{
    setWindowTitle("CPU usage");
    QPalette pal_ = this->palette();
    pal_.setColor(QPalette::Window, Qt::white);
    this->setAutoFillBackground(true);
    this->setPalette(pal_); // Белый цвет для всей формы
    QWidget * nainWidget = new QWidget;
    QVBoxLayout * nainVBoxLayout = new QVBoxLayout;
    cbMultyProc = new QCheckBox;
    cbMultyProc->setText("For each proc");
    // nainVBoxLayout->addWidget(cbMultyProc, 0, Qt::AlignHCenter);
    QGroupBox * gbProc = new QGroupBox;
    QPalette pal = gbProc->palette();
    pal.setColor(QPalette::Window, Qt::white);
    gbProc->setAutoFillBackground(true);
    gbProc->setPalette(pal); // Белый цвет для gbProc
    gbProc->setTitle("CPU usage");
    QVBoxLayout * vblProc = new QVBoxLayout;
    vblProc->addWidget(cbMultyProc, 0, Qt::AlignHCenter);


    cpProcStat->setMinimumSize(700, 300);
    cpProcStat->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    vblProc->addWidget(cpProcStat);
    gbProc->setLayout(vblProc);
    nainVBoxLayout->addWidget(gbProc, 10);

    nainWidget->setLayout(nainVBoxLayout);
    setCentralWidget(gbProc); // Поменять
    // setCentralWidget(nainWidget); // Поменять

    QCPAxis *yAxis = cpProcStat->yAxis;
    yAxis->setNumberFormat("f");
    yAxis->setNumberPrecision(0);
    PercentTicker *ticker = new PercentTicker;
    yAxis->setTicker(QSharedPointer<PercentTicker>(ticker));
    yAxis->ticker()->setTickStepStrategy(QCPAxisTicker::tssReadability);
    yAxis->setSubTicks(false);
    yAxis->ticker()->setTickCount(5);
    yAxis->setRange(0, 100);

    QSharedPointer<QCPAxisTickerDateTime> dateTimeTicker(new QCPAxisTickerDateTime);
    dateTimeTicker->setDateTimeFormat("hh:mm");
    cpProcStat->xAxis->setTicker(dateTimeTicker);
    cpProcStat->xAxis->setRange(80, 380);
    cpProcStat->xAxis->setSubTicks(false);
    cpProcStat->xAxis->setLabel("CPU Usage:");

    cpProcStat->addGraph();
    cpProcStat->graph(0)->setPen(QPen(Qt::blue));
    cpProcStat->graph(0)->setName("CPU Usage\n0.0%");
    QPen pen = cpProcStat->graph(0)->pen();
    pen.setWidth(3);
    cpProcStat->graph(0)->setPen(pen);
    cpProcStat->legend->itemWithPlottable(cpProcStat->graph(0))->setTextColor(Qt::blue);
    cpProcStat->yAxis->setRange(0, 100.);
    cpProcStat->legend->setVisible(true);
    cpProcStat->legend->setBorderPen(Qt::NoPen);
    cpProcStat->legend->setIconSize(10, 10);
    QCPLayoutGrid *subLayout = new QCPLayoutGrid;
    cpProcStat->plotLayout()->addElement(0, 1, subLayout);
    subLayout->addElement(0, 0, cpProcStat->legend);
    cpProcStat->legend->setMargins(QMargins(0, 0, 0, 0));
    cpProcStat->axisRect()->setAutoMargins(QCP::msLeft | QCP::msTop | QCP::msBottom);
    cpProcStat->plotLayout()->setColumnStretchFactor(1, 0.001);

    cpProcStat->replot();

    procReceiver->moveToThread(procThread);
    connect(procThread, &QThread::started, procReceiver, &clProcReceiver::slotOnStart, Qt::QueuedConnection);
    connect(procThread, &QThread::finished, procReceiver, &QObject::deleteLater);
    connect(procReceiver, &clProcReceiver::signalProcIsReseived, this, &MainWindow::slotGraphRepaint, Qt::QueuedConnection);
    procThread->start();
}

MainWindow::~MainWindow() {
    procThread->quit();
    procThread->wait();
}

void MainWindow::moveLegendFirstToEnd()
{
    QCPAbstractLegendItem  * abstractItem = cpProcStat->legend->item(0);
    QCPPlottableLegendItem * oldItem = qobject_cast<QCPPlottableLegendItem*>(abstractItem);
    QCPAbstractPlottable * plottable = oldItem->plottable();
    QColor textColor = oldItem->textColor();
    cpProcStat->legend->removeItem(oldItem);
    QCPPlottableLegendItem *newItem = new QCPPlottableLegendItem(cpProcStat->legend, plottable);
    newItem->setTextColor(textColor);
    cpProcStat->legend->addItem(newItem);
    cpProcStat->legend->item(procReceiver->countProc)->setVisible(false);
}

void MainWindow::slotGraphRepaint()
{
    cpProcStat->xAxis->setLabel("CPU Usage: " + QString::number(procReceiver->valueProc.last(), 'f', 1) + "%");
    cpProcStat->graph(0)->setName("CPU Usage\n" + QString::number(procReceiver->valueProc.last(), 'f', 1) + "%");
    if (cpProcStat->graphCount() == 1) {
        int iColor = 0;
        for (int i = 0; i < procReceiver->countProc; i++, iColor++) {
            cpProcStat->addGraph();
            if (iColor >= colorV.size()) iColor -= colorV.size();
            cpProcStat->graph(i + 1)->setPen(QPen(colorV[iColor]));
            QPen pen = cpProcStat->graph(i + 1)->pen();
            pen.setWidth(3);
            cpProcStat->graph(i + 1)->setPen(pen);
            cpProcStat->graph(i + 1)->setName("CPU" + QString::number(i));
            cpProcStat->legend->itemWithPlottable(cpProcStat->graph(i + 1))->setTextColor(colorV[iColor]);
        }
        for (int i = 1; i < cpProcStat->graphCount(); ++i) cpProcStat->legend->item(i)->setVisible(false);
    }
    if (cbMultyProc->isChecked()) {
        if (cpProcStat->graph(0)->visible()) {
            moveLegendFirstToEnd();
            for (int i = 0; i < procReceiver->countProc; i++) cpProcStat->legend->item(i)->setVisible(true);
        }
        cpProcStat->graph(0)->setVisible(false);
        for (int i = 0; i < procReceiver->countProc; i++) {
            cpProcStat->graph(i + 1)->setVisible(true);
            cpProcStat->graph(i + 1)->data()->clear();
            cpProcStat->graph(i + 1)->setData(procReceiver->vKey, procReceiver->vValueProc[i]);
            cpProcStat->xAxis->setRange(procReceiver->vKey.last() - 299, procReceiver->vKey.last());
            cpProcStat->yAxis->setRange(0, procReceiver->procMaxV);
        }
    }
    else {
        for (int i = 0; i < procReceiver->countProc; i++)
            cpProcStat->graph(i + 1)->setName("CPU Usage" + QString::number(i) + "\n"
                                              + QString::number( procReceiver->vValueProc[i].last(), 'f', 1) + "%");
        if (!cpProcStat->graph(0)->visible()) {
            for (int i = 0; i < procReceiver->countProc; i++) moveLegendFirstToEnd();
            cpProcStat->legend->item(0)->setVisible(true);
        }
        for (int i = 0; i < procReceiver->countProc; i++) cpProcStat->graph(i + 1)->setVisible(false);
        cpProcStat->graph(0)->setVisible(true);
        cpProcStat->graph(0)->data()->clear();
        cpProcStat->graph(0)->setData(procReceiver->vKey, procReceiver->valueProc);
        cpProcStat->xAxis->setRange(procReceiver->vKey.last() - 299, procReceiver->vKey.last());
        cpProcStat->yAxis->setRange(0, procReceiver->procMax);
    }
    cpProcStat->replot();
}
