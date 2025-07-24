cmake_minimum_required(VERSION 3.6)

project(kl520_ffi_lib)

# Set C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# === Include Directories ===
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include        # Kneron SDK headers
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src            # Kneron SDK sources
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/include/local
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/include/soc_common
    ${CMAKE_CURRENT_SOURCE_DIR}/../../ex_common      # Helper functions
)

file(GLOB LOCAL_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
)

file(GLOB KNERON_SDK_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/../../src/*.c"
)

file(GLOB EX_COMMON_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/../../ex_common/*.c"
)

add_library(
    kl520_ffi_lib
    SHARED
    ${LOCAL_SOURCES}
    ${KNERON_SDK_SOURCES}
    ${EX_COMMON_SOURCES}
)

find_library(log-lib log)

target_link_libraries(kl520_ffi_lib
    ${log-lib}
)

