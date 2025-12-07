cmake_minimum_required(VERSION 3.16)

project(DISRecorder LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(OpenDIS CONFIG REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Widgets Network)

add_executable(disrecorder
    src/MainWindow.cpp
    src/main.cpp
)

target_link_libraries(disrecorder PRIVATE Qt6::Widgets Qt6::Network OpenDIS::DIS6 OpenDIS::DIS7)
