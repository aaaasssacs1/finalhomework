cmake_minimum_required(VERSION 3.16)

project(PlcMonitor VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 查找 Qt6 包
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Charts Sql)

# 查找 Libmodbus (完全依赖 vcpkg 提供的配置模式)
find_package(libmodbus CONFIG REQUIRED)

include_directories(include ${MODBUS_INCLUDE_DIR})

# 源文件
set(SOURCES
    src/main.cpp
    src/MainWindow.cpp
    src/ModbusWorker.cpp
    src/DbManager.cpp
    include/MainWindow.h
    include/ModbusWorker.h
    include/DbManager.h
)

# 可执行文件
add_executable(${PROJECT_NAME} ${SOURCES})

# 链接库
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Charts
    Qt6::Sql
    modbus
)
