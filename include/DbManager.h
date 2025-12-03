#pragma once
#include <QSqlDatabase>
#include "ModbusWorker.h" // 引用 PlcData 结构体

class DbManager {
public:
    DbManager(const QString& path);
    ~DbManager();
    bool isOpen() const;

    // 修改插入函数，接收结构体
    bool insertData(const PlcData& data);

private:
    QSqlDatabase m_db;
};