#include "MainWindow.h"
#include "ModbusWorker.h"
#include "DbManager.h"

#include <QDebug>
#include <QDateTime>
#include <QStatusBar> 
#include <QGroupBox>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    m_db = new DbManager("plc_data.db");
    setupUi();
    setupObj();
    m_startTime = QDateTime::currentMSecsSinceEpoch();
}

MainWindow::~MainWindow() {
    if (m_worker) m_worker->stop();
    m_workerThread.quit();
    m_workerThread.wait();
    if (m_db) delete m_db;
}

void MainWindow::setupUi() {
    resize(900, 700);
    m_centralWidget = new QWidget(this);
    setCentralWidget(m_centralWidget);
    m_mainLayout = new QVBoxLayout(m_centralWidget);

    // --- 仪表盘区域 (水平布局) ---
    QHBoxLayout *gaugesLayout = new QHBoxLayout();

    // 1. 温度面板
    QGroupBox *tempGroup = new QGroupBox("Temperature (°C)", this);
    QVBoxLayout *tempLayout = new QVBoxLayout(tempGroup);
    m_tempGauge = new QDial(this);
    m_tempGauge->setRange(0, 100);
    m_tempGauge->setNotchesVisible(true);
    m_tempLcd = new QLCDNumber(this);
    m_tempLcd->setSegmentStyle(QLCDNumber::Flat);
    tempLayout->addWidget(m_tempGauge);
    tempLayout->addWidget(m_tempLcd);
    
    // 2. 压力面板 (新增)
    QGroupBox *presGroup = new QGroupBox("Pressure (Bar)", this);
    QVBoxLayout *presLayout = new QVBoxLayout(presGroup);
    m_presGauge = new QDial(this);
    m_presGauge->setRange(0, 20); // 假设压力范围 0-20 Bar
    m_presGauge->setNotchesVisible(true);
    // 给压力表盘换个颜色区分
    m_presGauge->setStyleSheet("background-color: #E0F7FA;"); 
    m_presLcd = new QLCDNumber(this);
    m_presLcd->setSegmentStyle(QLCDNumber::Flat);
    presLayout->addWidget(m_presGauge);
    presLayout->addWidget(m_presLcd);

    gaugesLayout->addWidget(tempGroup);
    gaugesLayout->addWidget(presGroup);
    m_mainLayout->addLayout(gaugesLayout);

    // --- 图表区域 ---
    setupChart(); 
    m_mainLayout->addWidget(m_chartView);

    // --- 状态栏 ---
    m_statusLabel = new QLabel("Ready", this);
    statusBar()->addWidget(m_statusLabel);
}

void MainWindow::setupChart() {
    m_chart = new QChart();
    m_chart->setTitle("PLC Real-time Monitor");

    // 温度曲线 (红色)
    m_seriesTemp = new QLineSeries();
    m_seriesTemp->setName("Temperature");
    m_seriesTemp->setColor(Qt::red);

    // 压力曲线 (蓝色) - 新增
    m_seriesPres = new QLineSeries();
    m_seriesPres->setName("Pressure");
    m_seriesPres->setColor(Qt::blue);

    m_chart->addSeries(m_seriesTemp);
    m_chart->addSeries(m_seriesPres);
    
    m_axisX = new QValueAxis();
    m_axisX->setTitleText("Time (s)");
    m_axisX->setRange(0, 30);
    
    m_axisY = new QValueAxis();
    m_axisY->setTitleText("Value");
    m_axisY->setRange(0, 100); // Y轴范围，如果压力很小，可能需要双Y轴

    m_chart->addAxis(m_axisX, Qt::AlignBottom);
    m_chart->addAxis(m_axisY, Qt::AlignLeft);
    
    m_seriesTemp->attachAxis(m_axisX);
    m_seriesTemp->attachAxis(m_axisY);
    m_seriesPres->attachAxis(m_axisX);
    m_seriesPres->attachAxis(m_axisY);

    m_chartView = new QChartView(m_chart);
    m_chartView->setRenderHint(QPainter::Antialiasing);
}

void MainWindow::setupObj() {
    m_worker = new ModbusWorker;
    m_worker->moveToThread(&m_workerThread);

    connect(&m_workerThread, &QThread::finished, m_worker, &QObject::deleteLater);
    connect(this, &MainWindow::operate, m_worker, &ModbusWorker::process);
    
    // 连接新定义的信号槽
    qRegisterMetaType<PlcData>("PlcData"); // 确保跨线程信号槽正常工作
    connect(m_worker, &ModbusWorker::dataRefreshed, this, &MainWindow::handlePlcData);
    
    connect(m_worker, &ModbusWorker::errorOccurred, this, &MainWindow::handleError);
    connect(m_worker, &ModbusWorker::connectionStatusChanged, this, [this](bool connected){
        m_statusLabel->setText(connected ? "PLC Connected" : "PLC Disconnected");
        m_statusLabel->setStyleSheet(connected ? "color: green;" : "color: red;");
    });
}

// 修改函数签名，增加 isSim
void MainWindow::setupConnection(const QString &ip, int port, bool isSim) {
    if (m_worker) {
        m_worker->setupConnection(ip, port);
        m_worker->setReadParameters(1, 0, 2);
        
        // --- 新增：设置模拟模式 ---
        m_worker->setSimulationMode(isSim);
    }
    if (!m_workerThread.isRunning()) {
        m_workerThread.start();
        emit operate();
    }
}

void MainWindow::handlePlcData(PlcData data) {
    if (!data.isValid) return;

    // 1. 更新温度 UI
    m_tempGauge->setValue(static_cast<int>(data.temperature));
    m_tempLcd->display(data.temperature);

    // 2. 更新压力 UI
    m_presGauge->setValue(static_cast<int>(data.pressure));
    m_presLcd->display(data.pressure);

    // 3. 更新图表
    double currentTime = (QDateTime::currentMSecsSinceEpoch() - m_startTime) / 1000.0;
    m_seriesTemp->append(currentTime, data.temperature);
    m_seriesPres->append(currentTime, data.pressure);

    // 滚动 X 轴
    if (currentTime > 30) {
        m_axisX->setRange(currentTime - 30, currentTime);
    }

    // 4. 存入数据库
    if (m_db) {
        m_db->insertData(data);
    }
    
    m_statusLabel->setText(QString("Temp: %1 C | Pres: %2 Bar").arg(data.temperature).arg(data.pressure));
}

void MainWindow::handleError(QString msg) {
    m_statusLabel->setText("Error: " + msg);
    m_statusLabel->setStyleSheet("color: red;");
}