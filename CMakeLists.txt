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
    Charts
)

set(SOURCES
    main.cpp
    DatabaseManager.cpp
    ProcessTracker.cpp
    MainWindow.cpp
    SettingsDialog.cpp
)

set(RESOURCES
    resources.qrc
)

add_executable(TimeTracker
    ${SOURCES}
    ${RESOURCES}
    DatabaseManager.h MainWindow.h ProcessTracker.h SettingsDialog.h
)

target_link_libraries(TimeTracker PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Sql
    Qt6::Charts
)

if(WIN32)
    target_link_libraries(TimeTracker PRIVATE
        user32
        advapi32
    )
endif()

set_target_properties(TimeTracker PROPERTIES
    WIN32_EXECUTABLE TRUE
)
