cmake_minimum_required(VERSION 3.19)

list(APPEND CMAKE_PREFIX_PATH "C:/dev/FinanceTracker/vcpkg_installed/x64-windows")

project(FinanceTracker LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Charts Sql)
find_package(PostgreSQL REQUIRED)

qt_standard_project_setup()

qt_add_executable(FinanceTracker
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    db/DatabaseManager.hpp
    db/DatabaseManager.cpp
    db/migrations/001_initial_schema.sql
    vcpkg.json
)

target_link_libraries(FinanceTracker
    PRIVATE
        Qt::Core
        Qt::Widgets
        Qt::Charts
        Qt::Sql
        PostgreSQL::PostgreSQL
)

install(TARGETS FinanceTracker
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET FinanceTracker
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
