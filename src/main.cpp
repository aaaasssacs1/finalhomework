#include <QApplication>
#include "MainWindow.h"
#include <QStringList>
#include <QDebug>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setStyle("Fusion");

    // === 1. 设置默认行为 ===
    // 默认开启模拟模式，防止小白用户直接运行报错
    bool useSim = true; 
    
    // 默认显示 IP，实际在模拟模式下不连接，但这用于窗口标题显示
    QString targetIp = "Local Simulation"; 
    int targetPort = 502;

    // === 2. 解析命令行参数 ===
    QStringList args = app.arguments();
    bool userProvidedIp = false;

    for (int i = 1; i < args.size(); ++i) {
        QString arg = args[i];

        // 跳过 Qt 平台参数 (如 -platform linuxfb)
        if (arg == "-platform" || (i > 1 && args[i-1] == "-platform")) {
            continue; 
        }
        
        // 跳过 flag 参数
        if (arg.startsWith("-")) {
            continue;
        }

        // 简单的检测：如果参数包含 '.' 或 ':'，我们认为用户输入了 IP 地址
        // 此时，我们假设用户想进行真实连接
        if (arg.contains(".") || arg.contains(":")) {
            userProvidedIp = true;
            useSim = false; // <--- 关键：用户指定 IP 后，自动关闭模拟模式
            
            if (arg.contains(":")) {
                QStringList parts = arg.split(':');
                targetIp = parts[0];
                if (parts.size() > 1) {
                    targetPort = parts[1].toInt();
                }
            } else {
                targetIp = arg;
            }
        }
    }

    // 允许通过显式参数强制覆盖
    if (args.contains("--sim")) {
        useSim = true;
    }
    if (args.contains("--real")) {
        useSim = false;
        if (!userProvidedIp) targetIp = "127.0.0.1";
    }

    // === 3. 打印启动信息 ===
    qDebug() << "------------------------------------------------";
    qDebug() << "Starting PLC Monitor";
    qDebug() << "Mode            :" << (useSim ? "SIMULATION (Generated Data)" : "REAL MODBUS TCP");
    if (!useSim) {
        qDebug() << "Target PLC IP   :" << targetIp;
        qDebug() << "Target PLC Port :" << targetPort;
    }
    qDebug() << "------------------------------------------------";

    MainWindow w;
    
    // 设置窗口标题
    QString title = useSim ? QString("PLC Monitor - Simulation Mode") 
                           : QString("PLC Monitor - %1:%2").arg(targetIp).arg(targetPort);
    w.setWindowTitle(title);
    
    // 传递参数：IP, Port, isSim
    w.setupConnection(targetIp, targetPort, useSim);

    w.show();

    return app.exec();
}