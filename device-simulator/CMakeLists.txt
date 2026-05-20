add_executable(device-simulator
    src/main.cpp
    src/app/SimulatorApplication.cpp
    src/net/JsonTcpServer.cpp
)

target_link_libraries(device-simulator PRIVATE scada_common)

target_compile_features(device-simulator PRIVATE cxx_std_11)

target_include_directories(device-simulator PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# MySQL 路径发现（与 master-server 相同的逻辑）
if(DEFINED MYSQL_PATH)
    if(EXISTS "${MYSQL_PATH}/include/mysql.h")
        set(SIM_MYSQL_INCLUDE_DIRS "${MYSQL_PATH}/include")
    elseif(EXISTS "${MYSQL_PATH}/include/mysql/mysql.h")
        set(SIM_MYSQL_INCLUDE_DIRS "${MYSQL_PATH}/include/mysql")
    endif()
    if(EXISTS "${MYSQL_PATH}/lib/libmysql.lib")
        set(SIM_MYSQL_LIBRARIES "${MYSQL_PATH}/lib/libmysql.lib")
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
            set(SIM_MYSQL_INCLUDE_DIRS "${MYSQL_PATH}/include")
        elseif(EXISTS "${MYSQL_PATH}/include/mysql/mysql.h")
            set(SIM_MYSQL_INCLUDE_DIRS "${MYSQL_PATH}/include/mysql")
        endif()
        if(SIM_MYSQL_INCLUDE_DIRS)
            if(EXISTS "${MYSQL_PATH}/lib/libmysql.lib")
                set(SIM_MYSQL_LIBRARIES "${MYSQL_PATH}/lib/libmysql.lib")
            endif()
            break()
        endif()
    endforeach()
endif()

if(DEFINED SIM_MYSQL_LIBRARIES)
    target_link_libraries(device-simulator PRIVATE ${SIM_MYSQL_LIBRARIES})
    target_include_directories(device-simulator PRIVATE ${SIM_MYSQL_INCLUDE_DIRS})
    if(WIN32)
        target_link_libraries(device-simulator PRIVATE ws2_32)
    endif()
    message(STATUS "device-simulator: MySQL found at ${SIM_MYSQL_INCLUDE_DIRS}")
else()
    message(WARNING "device-simulator: MySQL not found - will use hardcoded defaults")
endif()

# Windows 特定配置
if(WIN32)
    target_link_libraries(device-simulator PRIVATE ws2_32)
endif()
