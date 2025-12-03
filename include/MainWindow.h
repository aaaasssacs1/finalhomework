#pragma once
#include <QMainWindow>
#include <QThread>
#include <QLabel>
#include <QBoxLayout>
#include <QDial>
#include <QLCDNumber>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include "ModbusWorker.h" // 需要用到 PlcData

class DbManager;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void setupConnection(const QString &ip, int port, bool isSim = false);

signals:
    void operate();

private slots:
    // 修改槽函数参数类型
    void handlePlcData(PlcData data); 
    void handleError(QString msg);

private:
    void setupUi();
    void setupObj();
    void setupChart();

private:
    QWidget *m_centralWidget;
    QVBoxLayout *m_mainLayout;
    QLabel *m_statusLabel;
    
    // --- 温度组件 ---
    QDial *m_tempGauge;
    QLCDNumber *m_tempLcd;
    QLabel *m_tempLabel;

    // --- 压力组件 (新增) ---
    QDial *m_presGauge;
    QLCDNumber *m_presLcd;
    QLabel *m_presLabel;

    // --- 图表组件 ---
    QChart *m_chart;
    QLineSeries *m_seriesTemp;
    QLineSeries *m_seriesPres; // 新增压力曲线
    QChartView *m_chartView;
    QValueAxis *m_axisX;
    QValueAxis *m_axisY;

    QThread m_workerThread;
    ModbusWorker *m_worker = nullptr;
    DbManager *m_db = nullptr;
    qint64 m_startTime;
};