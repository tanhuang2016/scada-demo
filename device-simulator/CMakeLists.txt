add_executable(device-simulator
    src/main.cpp
    src/app/SimulatorApplication.cpp
)

target_link_libraries(device-simulator PRIVATE scada_common)

target_compile_features(device-simulator PRIVATE cxx_std_17)

target_include_directories(device-simulator PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
