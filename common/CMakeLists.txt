add_library(scada_common STATIC
    src/protocol.cpp
)

target_include_directories(scada_common PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(scada_common PUBLIC cxx_std_17)
