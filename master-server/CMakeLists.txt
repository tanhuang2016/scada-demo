add_executable(master-server
    src/main.cpp
    src/app/MasterApplication.cpp
)

target_link_libraries(master-server PRIVATE scada_common)

target_compile_features(master-server PRIVATE cxx_std_17)

target_include_directories(master-server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
