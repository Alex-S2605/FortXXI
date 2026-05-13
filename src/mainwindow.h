#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QCheckBox>
#include <QLabel>
#include <QThread>

#include "qcustomplot.h"
#include "clprocreceiver.h"

class PercentTicker : public QCPAxisTicker
{
protected:
    virtual QString getTickLabel(double tick, const QLocale &locale,
                                 QChar formatChar, int precision) override
    {
        QString label = QCPAxisTicker::getTickLabel(tick, locale, formatChar, precision);
        return label + ", %";
    }
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    QCheckBox *cbMultyProc;
    QCustomPlot *cpProcStat;
    clProcReceiver * procReceiver;
    QThread * procThread;
    void moveLegendFirstToEnd();

private slots:
    void slotGraphRepaint();

};
#endif // MAINWINDOW_H
