cmake_minimum_required(VERSION 3.16)

project(scada-demo VERSION 1.0 LANGUAGES CXX)

# 强制源文件和字串字面量为 UTF-8
# GCC/MinGW 需要显式指定，否则 UTF-8 源码中的中文会被转成系统 GBK
# MSVC 用 /utf-8（VS2015+）
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    add_compile_options(-finput-charset=UTF-8 -fexec-charset=UTF-8)
elseif(MSVC)
    add_compile_options(/utf-8)
endif()

# 全项目统一 C++11（与目标岗位/现有规范一致）
set(CMAKE_CXX_STANDARD 11)
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

# 自动检测常见的 Qt 5 安装路径（Windows）
if(WIN32 AND NOT CMAKE_PREFIX_PATH)
    # 常见路径列表
    set(QT5_POSSIBLE_PATHS
        "D:/soft/Qt/5.15.2/mingw81_64"
        "C:/Qt/5.15.2/mingw81_64"
        "C:/Qt5/5.15.2/mingw81_64"
    )

    foreach(QT_PATH ${QT5_POSSIBLE_PATHS})
        if(EXISTS ${QT_PATH}/lib/cmake/Qt5/Qt5Config.cmake)
            message(STATUS "Found Qt 5 at: ${QT_PATH}")
            list(APPEND CMAKE_PREFIX_PATH ${QT_PATH})
            break()
        endif()
    endforeach()
endif()

# Windows 统一预定义：避免 winsock.h / windows.h 冲突
if(WIN32)
    add_compile_definitions(WIN32_LEAN_AND_MEAN _WINSOCK_DEPRECATED_NO_WARNINGS)
endif()

# 子项目：common 必须先于各可执行文件
add_subdirectory(common)
add_subdirectory(master-server)
add_subdirectory(device-simulator)
add_subdirectory(qt-client)