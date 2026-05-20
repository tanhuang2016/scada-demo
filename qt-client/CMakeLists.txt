set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 告诉 AUTOUIC 在哪里查找 .ui 文件
set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_CURRENT_SOURCE_DIR}/ui)

find_package(Qt5 REQUIRED COMPONENTS Core Widgets Network)

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
    src/storage/DeviceManager.cpp
    src/storage/DeviceManager.hpp
    ui/MainWindow.ui
    ui/MonitorPage.ui
    ui/DeviceManagePage.ui
    ui/DeviceFormDialog.ui
)

# MySQL 路径发现（与 master-server 相同的逻辑）
# 优先使用用户指定的 MYSQL_PATH
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
    # 尝试常见位置
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

# 链接 MySQL（如果找到）
if(DEFINED QT_MYSQL_LIBRARIES)
    target_link_libraries(qt-client PRIVATE ${QT_MYSQL_LIBRARIES})
    target_include_directories(qt-client PRIVATE ${QT_MYSQL_INCLUDE_DIRS})
    if(WIN32)
        target_link_libraries(qt-client PRIVATE ws2_32)
    endif()
    message(STATUS "qt-client: MySQL found at ${QT_MYSQL_INCLUDE_DIRS}")
else()
    message(WARNING "qt-client: MySQL not found - device management page will not compile")
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
        DOC "Qt deployment tool for Windows"
    )

    if(WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET qt-client POST_BUILD
            COMMAND ${WINDEPLOYQT_EXECUTABLE}
                --no-translations
                --no-system-d3d-compiler
                --no-opengl-sw
                $<TARGET_FILE:qt-client>
            COMMENT "Deploying Qt dependencies..."
        )
    else()
        message(WARNING "windeployqt not found.")
    endif()
endif()
