#include "DbManager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QDateTime> 

DbManager::DbManager(const QString& path) {
    m_db = QSqlDatabase::addDatabase("QSQLITE");
    m_db.setDatabaseName(path);

    if (m_db.open()) {
        QSqlQuery query;
        // 修改表结构：增加 pressure 字段
        // 注意：如果之前已经运行过程序生成了 db 文件，需要手动删除 db 文件或修改 SQL 进行 ALTER TABLE
        bool success = query.exec("CREATE TABLE IF NOT EXISTS plc_logs ("
                                  "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                                  "timestamp DATETIME, "
                                  "temperature REAL, "
                                  "pressure REAL)"); 
        if (!success) qDebug() << "Create table error:" << query.lastError();
    }
}

DbManager::~DbManager() {
    if (m_db.isOpen()) m_db.close();
}

bool DbManager::isOpen() const { return m_db.isOpen(); }

bool DbManager::insertData(const PlcData& data) {
    if (!m_db.isOpen()) return false;

    QSqlQuery query;
    query.prepare("INSERT INTO plc_logs (timestamp, temperature, pressure) "
                  "VALUES (:time, :temp, :pres)");
    query.bindValue(":time", QDateTime::currentDateTime());
    query.bindValue(":temp", data.temperature);
    query.bindValue(":pres", data.pressure);

    return query.exec();
}