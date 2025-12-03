#include "ModbusWorker.h"
#include <QThread>
#include <QDebug>
#include <QRandomGenerator>
#include <cerrno>

ModbusWorker::ModbusWorker(QObject *parent) 
    : QObject(parent), m_running(false), m_ctx(nullptr) 
{
    // 默认参数
    m_slaveId = 1;
    m_startAddress = 0; 
    m_numRegisters = 2; // 默认读取2个寄存器（温度+压力）
}

ModbusWorker::~ModbusWorker() {
    stop();
    if (m_ctx) {
        modbus_close(m_ctx);
        modbus_free(m_ctx);
    }
}

void ModbusWorker::setupConnection(const QString &ip, int port) {
    m_ip = ip;
    m_port = port;
}

void ModbusWorker::setReadParameters(int slaveId, int startAddress, int count) {
    m_slaveId = slaveId;
    m_startAddress = startAddress;
    m_numRegisters = count;
}

void ModbusWorker::stop() {
    m_running = false;
}

void ModbusWorker::process() {
    m_running = true;

    // 1. 初始化 Context
    if (!m_simulationMode) {
        QByteArray ipBytes = m_ip.toUtf8();
        m_ctx = modbus_new_tcp(ipBytes.constData(), m_port);
        
        if (m_ctx == nullptr) {
            emit errorOccurred("Unable to allocate libmodbus context");
            return;
        }

        // 设置超时时间：2秒
        modbus_set_response_timeout(m_ctx, 2, 0);
    }

    // 动态分配内存
    uint16_t *tab_reg = new uint16_t[m_numRegisters]; 
    bool isConnected = false;

    // === 主循环 ===
    while (m_running) {
        
        // --- A. 模拟模式逻辑 ---
        if (m_simulationMode) {
            PlcData simData;
            simData.temperature = 20.0 + QRandomGenerator::global()->generateDouble() * 10.0;
            // 模拟 5-7 Bar 的压力
            simData.pressure = 5.0 + QRandomGenerator::global()->generateDouble() * 2.0;
            simData.isValid = true;

            emit dataRefreshed(simData);
            emit connectionStatusChanged(true);
            QThread::msleep(1000);
            continue;
        }

        // --- B. 真实 Modbus 逻辑 ---
        
        // 1. 自动重连机制
        if (!isConnected) {
            if (modbus_connect(m_ctx) == -1) {
                QString err = QString("Connection failed: %1").arg(modbus_strerror(errno));
                qWarning() << err; 
                emit connectionStatusChanged(false);
                
                // 连接失败，分段休眠以便响应 stop()
                for (int i=0; i<20 && m_running; i++) QThread::msleep(100);
                continue; 
            } else {
                qDebug() << "Connected to PLC at" << m_ip;
                modbus_set_slave(m_ctx, m_slaveId);
                isConnected = true;
                emit connectionStatusChanged(true);
            }
        }

        // 2. 读取数据
        int rc = modbus_read_registers(m_ctx, m_startAddress, m_numRegisters, tab_reg);

        if (rc == -1) {
            // --- 读取失败 ---
            QString errMsg = modbus_strerror(errno);
            qWarning() << "Read Error:" << errMsg;
            
            emit errorOccurred("Read error: " + errMsg);
            emit connectionStatusChanged(false);

            // 主动断开，触发重连
            modbus_close(m_ctx);
            isConnected = false;
            
            QThread::sleep(1);
        } else {
            // --- 读取成功 ---
            PlcData data;
            
            // 假设寄存器 0 是温度 (PLC数值/10.0)
            if (m_numRegisters > 0) {
                data.temperature = static_cast<double>(tab_reg[0]) / 10.0;
            }
            
            // 假设寄存器 1 是压力 (PLC数值/10.0)
            if (m_numRegisters > 1) {
                data.pressure = static_cast<double>(tab_reg[1]) / 10.0;
            } else {
                data.pressure = 0.0;
            }
            
            data.isValid = true;
            emit dataRefreshed(data);
            
            // 采样间隔 500ms
            QThread::msleep(500);
        }
    }

    // === 清理 ===
    delete[] tab_reg;
    if (m_ctx) {
        modbus_close(m_ctx);
        modbus_free(m_ctx);
        m_ctx = nullptr;
    }
    qDebug() << "ModbusWorker thread finished.";
}

// 确保在 ModbusWorker.cpp 中添加这个函数
void ModbusWorker::setSimulationMode(bool active) {
    m_simulationMode = active;
}