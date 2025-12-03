#pragma once
#include <QObject>
#include <modbus/modbus.h>

// 定义一个结构体包含所有传感器数据
struct PlcData {
    double temperature; // 温度
    double pressure;    // 压力 (新增)
    bool isValid;       // 数据是否有效
};

// 为了让 Qt 信号槽能传递结构体，需注册类型（在 cpp 中注册或使用 QVariant，这里简单处理直接传值）
Q_DECLARE_METATYPE(PlcData)

class ModbusWorker : public QObject {
    Q_OBJECT
public:
    explicit ModbusWorker(QObject *parent = nullptr);
    ~ModbusWorker();

    void setupConnection(const QString &ip, int port);
    void setReadParameters(int slaveId, int startAddress, int count);
    void process();
    void stop();
    void setSimulationMode(bool active);

signals:
    // 修改信号：传递结构体数据，而不是单个 double
    void dataRefreshed(PlcData data); 
    
    void errorOccurred(QString msg);
    void connectionStatusChanged(bool connected);

private:
    modbus_t *m_ctx = nullptr;
    bool m_running;
    bool m_simulationMode = false;
    
    QString m_ip;
    int m_port;

    int m_slaveId = 1;
    int m_startAddress = 0;
    int m_numRegisters = 2; // 改为 2：读取温度(0) + 压力(1)
};
