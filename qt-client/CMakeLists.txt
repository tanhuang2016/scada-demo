set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 REQUIRED COMPONENTS Core Widgets Network)

add_executable(qt-client
    src/main.cpp
    src/MainWindow.cpp
    ui/MainWindow.ui
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
)
