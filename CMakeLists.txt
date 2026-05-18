cmake_minimum_required(VERSION 3.16)

project(scada-demo VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 输出目录统一
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Qt 自动处理由 qt-client/CMakeLists.txt 按需开启

# 可通过 -DCMAKE_PREFIX_PATH= 指定 Qt 安装路径
if(DEFINED ENV{QTDIR})
    list(APPEND CMAKE_PREFIX_PATH $ENV{QTDIR})
endif()

# 子项目：common 必须先于各可执行文件
add_subdirectory(common)
add_subdirectory(master-server)
add_subdirectory(device-simulator)
add_subdirectory(qt-client)