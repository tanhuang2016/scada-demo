add_library(scada_common STATIC
    src/protocol.cpp
    src/json_protocol.cpp
    src/iec104_protocol.cpp
    src/protocol_factory.cpp
)

target_include_directories(scada_common PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(scada_common PUBLIC cxx_std_11)
