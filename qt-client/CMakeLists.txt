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
    ui/MainWindow.ui
    ui/MonitorPage.ui
)

target_link_libraries(qt-client PRIVATE
    scada_common
    Qt5::Core
    Qt5::Widgets
    Qt5::Network
)

target_compile_features(qt-client PRIVATE cxx_std_11)

target_include_directories(qt-client PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}  # 添加构建目录到包含路径，用于查找生成的 ui_MainWindow.h
)

# 自动部署 Qt DLL 到可执行文件目录（Windows 上）
if(WIN32)
    # 查找 windeployqt 工具
    get_target_property(QT_QMAKE_EXECUTABLE Qt5::qmake LOCATION)
    get_filename_component(QT_BIN_DIR ${QT_QMAKE_EXECUTABLE} DIRECTORY)
    find_program(WINDEPLOYQT_EXECUTABLE
        NAMES windeployqt
        PATHS ${QT_BIN_DIR}
        DOC "Qt deployment tool for Windows"
    )

    if(WINDEPLOYQT_EXECUTABLE)
        # 添加自定义目标来部署依赖
        add_custom_command(TARGET qt-client POST_BUILD
            COMMAND ${WINDEPLOYQT_EXECUTABLE}
                --no-translations
                --no-system-d3d-compiler
                --no-opengl-sw
                $<TARGET_FILE:qt-client>
            COMMENT "Deploying Qt dependencies..."
        )
    else()
        message(WARNING "windeployqt not found. You may need to manually copy Qt DLLs to the executable directory.")
    endif()
endif()
