add_executable(master-server
    src/main.cpp
    src/app/MasterApplication.cpp
    src/storage/MySQLConnection.cpp
    src/storage/DeviceRepository.cpp
)

# 用户通过 CMake 变量指定 MySQL Connector/C 路径：-DMYSQL_PATH="D:/soft/mysql/mysql-connector-c-6.1.11-winx64"
if(DEFINED MYSQL_PATH)
    message(STATUS "Using MySQL Connector/C path from user: ${MYSQL_PATH}")
    # 查找头文件
    if(EXISTS "${MYSQL_PATH}/include/mysql.h")
        set(MYSQL_INCLUDE_DIRS "${MYSQL_PATH}/include")
    elseif(EXISTS "${MYSQL_PATH}/include/mysql/mysql.h")
        set(MYSQL_INCLUDE_DIRS "${MYSQL_PATH}/include/mysql")
    else()
        message(FATAL_ERROR "MySQL headers not found in ${MYSQL_PATH}/include")
    endif()
    # 查找库文件
    if(EXISTS "${MYSQL_PATH}/lib/libmysql.lib")
        set(MYSQL_LIBRARIES "${MYSQL_PATH}/lib/libmysql.lib")
    else()
        message(FATAL_ERROR "MySQL library not found in ${MYSQL_PATH}/lib/libmysql.lib")
    endif()
else()
    # 尝试常见位置（包括 Connector/C）
    set(MYSQL_POSSIBLE_PATHS
        "D:/soft/mysql/mysql-connector-c-6.1.11-winx64"
        "C:/Program Files/MySQL/MySQL Server 8.0"
        "C:/Program Files (x86)/MySQL/MySQL Server 8.0"
        "D:/Program Files/MySQL/MySQL Server 8.0"
        "E:/Program Files/MySQL/MySQL Server 8.0"
        "D:/soft/mysql/mysql-8.0.39-winx64"
        "E:/soft/mysql/mysql-8.0.39-winx64"
    )

    foreach(MYSQL_PATH ${MYSQL_POSSIBLE_PATHS})
        if(EXISTS "${MYSQL_PATH}/include/mysql.h" OR EXISTS "${MYSQL_PATH}/include/mysql/mysql.h")
            message(STATUS "Found MySQL at: ${MYSQL_PATH}")
            if(EXISTS "${MYSQL_PATH}/include/mysql.h")
                set(MYSQL_INCLUDE_DIRS "${MYSQL_PATH}/include")
            else()
                set(MYSQL_INCLUDE_DIRS "${MYSQL_PATH}/include/mysql")
            endif()
            if(EXISTS "${MYSQL_PATH}/lib/libmysql.lib")
                set(MYSQL_LIBRARIES "${MYSQL_PATH}/lib/libmysql.lib")
            endif()
            break()
        endif()
    endforeach()
endif()

# 验证找到 MySQL
if(NOT DEFINED MYSQL_INCLUDE_DIRS OR NOT DEFINED MYSQL_LIBRARIES)
    message(FATAL_ERROR "MySQL libraries not found. Please specify MySQL path with -DMYSQL_PATH=\"D:/path/to/mysql/connector/c\"")
endif()

# 链接 MySQL 库和 common 库
target_link_libraries(master-server PRIVATE
    scada_common
    ${MYSQL_LIBRARIES}
)

# 添加 MySQL 头文件路径
target_include_directories(master-server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${MYSQL_INCLUDE_DIRS}
)

# Windows 特定配置
if(WIN32)
    # 复制 libmysql.dll 到输出目录
    if(EXISTS "${MYSQL_INCLUDE_DIRS}/../lib/libmysql.dll")
        set(MYSQL_DLL_PATH "${MYSQL_INCLUDE_DIRS}/../lib/libmysql.dll")
    elseif(EXISTS "${MYSQL_INCLUDE_DIRS}/libmysql.dll")
        set(MYSQL_DLL_PATH "${MYSQL_INCLUDE_DIRS}/libmysql.dll")
    else()
        message(WARNING "libmysql.dll not found - you may need to copy it manually to bin directory")
    endif()

    if(DEFINED MYSQL_DLL_PATH)
        add_custom_command(TARGET master-server POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
            "${MYSQL_DLL_PATH}"
            $<TARGET_FILE_DIR:master-server>
            COMMENT "Copied libmysql.dll to output directory"
        )
    endif()

    # 链接 Winsock 库
    target_link_libraries(master-server PRIVATE ws2_32)
    target_compile_definitions(master-server PRIVATE
        WIN32_LEAN_AND_MEAN
        _WINSOCK_DEPRECATED_NO_WARNINGS
    )
endif()

target_compile_features(master-server PRIVATE cxx_std_11)
