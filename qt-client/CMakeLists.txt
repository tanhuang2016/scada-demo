set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_CURRENT_SOURCE_DIR}/ui)

find_package(Qt5 REQUIRED COMPONENTS Core Widgets Network)

# Qt Charts 可选（可能未安装）
find_package(Qt5Charts QUIET)
if(Qt5Charts_FOUND)
    set(HAS_QT_CHARTS TRUE)
    message(STATUS "qt-client: Qt Charts found")
else()
    message(STATUS "qt-client: Qt Charts NOT found - history chart disabled")
endif()

add_executable(qt-client
    src/main.cpp
    src/MainWindow.cpp
    src/MainWindow.hpp
    src/net/MasterClient.cpp
    src/net/MasterClient.hpp
    src/pages/MonitorPage.cpp
    src/pages/MonitorPage.hpp
    src/pages/DeviceManagePage.cpp
    src/pages/DeviceManagePage.hpp
    src/pages/DeviceFormDialog.cpp
    src/pages/DeviceFormDialog.hpp
    src/pages/LoginDialog.cpp
    src/pages/LoginDialog.hpp
    src/pages/OperationLogPage.cpp
    src/pages/OperationLogPage.hpp
    src/pages/AlarmPage.cpp
    src/pages/AlarmPage.hpp
    src/storage/DeviceManager.cpp
    src/storage/DeviceManager.hpp
    ui/MainWindow.ui
    ui/MonitorPage.ui
    ui/DeviceManagePage.ui
    ui/DeviceFormDialog.ui
    ui/LoginDialog.ui
    ui/OperationLogPage.ui
    ui/AlarmPage.ui
)

# HistoryChartPage 仅在 Qt Charts 可用时编译
if(HAS_QT_CHARTS)
    target_sources(qt-client PRIVATE
        src/pages/HistoryChartPage.cpp
        src/pages/HistoryChartPage.hpp
        ui/HistoryChartPage.ui
    )
    target_compile_definitions(qt-client PRIVATE HAS_QT_CHARTS)
endif()

# MySQL 路径发现
if(DEFINED MYSQL_PATH)
    if(EXISTS "${MYSQL_PATH}/include/mysql.h")
        set(QT_MYSQL_INCLUDE_DIRS "${MYSQL_PATH}/include")
    elseif(EXISTS "${MYSQL_PATH}/include/mysql/mysql.h")
        set(QT_MYSQL_INCLUDE_DIRS "${MYSQL_PATH}/include/mysql")
    endif()
    if(EXISTS "${MYSQL_PATH}/lib/libmysql.lib")
        set(QT_MYSQL_LIBRARIES "${MYSQL_PATH}/lib/libmysql.lib")
    endif()
else()
    set(MYSQL_PATHS
        "${CMAKE_SOURCE_DIR}/external/mysql"
        "D:/soft/mysql/mysql-connector-c-6.1.11-winx64"
        "C:/Program Files/MySQL/MySQL Server 8.0"
        "D:/soft/mysql/mysql-8.0.39-winx64"
        "E:/soft/mysql/mysql-8.0.39-winx64"
    )
    foreach(MYSQL_PATH ${MYSQL_PATHS})
        if(EXISTS "${MYSQL_PATH}/include/mysql.h")
            set(QT_MYSQL_INCLUDE_DIRS "${MYSQL_PATH}/include")
        elseif(EXISTS "${MYSQL_PATH}/include/mysql/mysql.h")
            set(QT_MYSQL_INCLUDE_DIRS "${MYSQL_PATH}/include/mysql")
        endif()
        if(QT_MYSQL_INCLUDE_DIRS)
            if(EXISTS "${MYSQL_PATH}/lib/libmysql.lib")
                set(QT_MYSQL_LIBRARIES "${MYSQL_PATH}/lib/libmysql.lib")
            endif()
            break()
        endif()
    endforeach()
endif()

target_link_libraries(qt-client PRIVATE
    scada_common
    Qt5::Core
    Qt5::Widgets
    Qt5::Network
)

if(HAS_QT_CHARTS)
    target_link_libraries(qt-client PRIVATE Qt5::Charts)
endif()

if(DEFINED QT_MYSQL_LIBRARIES)
    target_link_libraries(qt-client PRIVATE ${QT_MYSQL_LIBRARIES})
    target_include_directories(qt-client PRIVATE ${QT_MYSQL_INCLUDE_DIRS})
    if(WIN32)
        target_link_libraries(qt-client PRIVATE ws2_32)
    endif()
    message(STATUS "qt-client: MySQL found at ${QT_MYSQL_INCLUDE_DIRS}")
else()
    message(WARNING "qt-client: MySQL not found")
endif()

target_compile_features(qt-client PRIVATE cxx_std_11)

target_include_directories(qt-client PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
)

# 自动部署 Qt DLL
if(WIN32)
    get_target_property(QT_QMAKE_EXECUTABLE Qt5::qmake LOCATION)
    get_filename_component(QT_BIN_DIR ${QT_QMAKE_EXECUTABLE} DIRECTORY)
    find_program(WINDEPLOYQT_EXECUTABLE
        NAMES windeployqt
        PATHS ${QT_BIN_DIR}
    )
    if(WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET qt-client POST_BUILD
            COMMAND ${WINDEPLOYQT_EXECUTABLE}
                --no-translations --no-system-d3d-compiler --no-opengl-sw
                $<TARGET_FILE:qt-client>
            COMMENT "Deploying Qt dependencies..."
        )
    endif()
endif()
