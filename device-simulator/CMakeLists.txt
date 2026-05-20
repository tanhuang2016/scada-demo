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

# Windows 特定配置
if(WIN32)
    target_link_libraries(device-simulator PRIVATE ws2_32)
endif()
