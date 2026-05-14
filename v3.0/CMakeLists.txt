cmake_minimum_required(VERSION 3.19)
project(TimeTracker LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Sql
    Svg
)

set(SOURCES
    main.cpp
    database.cpp
    tracker.cpp
    hourlychartwidget.cpp
    mainwindow.cpp
)

set(HEADERS
    database.h
    tracker.h
    hourlychartwidget.h
    mainwindow.h
)

set(RESOURCES
    resources.qrc
)

add_executable(TimeTracker
    ${SOURCES}
    ${RESOURCES}
    tubiao.rc
    database.h
    hourlychartwidget.h mainwindow.h tracker.h
)

target_link_libraries(TimeTracker PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Sql
    Qt6::Svg
)

if(WIN32)
    target_link_libraries(TimeTracker PRIVATE
        user32
        advapi32
        psapi
    )
endif()

set_target_properties(TimeTracker PROPERTIES
    WIN32_EXECUTABLE TRUE
)
