cmake_minimum_required(VERSION 3.24)

project(openpunchclock VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wno-unused-parameter)
endif()

if(ANDROID)
    find_package(Qt6 REQUIRED COMPONENTS Concurrent Core Linguist Network Quick QuickControls2 Sql WebSockets)
else()
    find_package(Qt6 REQUIRED COMPONENTS Concurrent Core Linguist Network Quick QuickControls2 Sql WebSockets Test)
    find_package(Qt6 OPTIONAL_COMPONENTS Widgets)
endif()

find_package(Qt6 OPTIONAL_COMPONENTS Positioning)
if(TARGET Qt6::Positioning)
    set(PUNCH_HAS_GPS ON)
else()
    set(PUNCH_HAS_GPS OFF)
    message(STATUS "Qt Positioning absent → GPS désactivé")
endif()

qt_standard_project_setup()

add_subdirectory(src)

if(NOT ANDROID)
    enable_testing()
    add_subdirectory(tests)
endif()
