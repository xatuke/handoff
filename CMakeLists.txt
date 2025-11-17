cmake_minimum_required(VERSION 3.16)
project(airpods-handoff)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Bluetooth DBus)

add_executable(airpods-handoff
    main.cpp
    media/mediacontroller.cpp
)

target_link_libraries(airpods-handoff
    Qt6::Core
    Qt6::Bluetooth
    Qt6::DBus
)

target_include_directories(airpods-handoff PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
