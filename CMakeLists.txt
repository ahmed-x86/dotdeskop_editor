cmake_minimum_required(VERSION 3.16)
project(dotdesktop_editor VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

# Find Qt (prefer Qt6, fall back to Qt5)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Gui Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets)

message(STATUS "Building with Qt ${QT_VERSION}")

add_executable(dotdesktop_editor main.cpp)

target_link_libraries(dotdesktop_editor PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
)
