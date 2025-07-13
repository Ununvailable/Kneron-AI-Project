cmake_minimum_required(VERSION 3.6)

project(kl520_ffi_lib)

# Set C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include  # Kneron SDK include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../ex_common # Common helper functions
    # ${OpenCV_INCLUDE_DIRS}
)

# === Source Files ===
file(GLOB LOCAL_SOURCES
    "*.cpp"
    "*.c"
)

# === Create Shared Library ===
add_library(
    kl520_ffi_lib
    SHARED
    ${LOCAL_SOURCES}
)

# === External Library Link Example (Testing) ===

# OpenCV
# find_package(OpenCV REQUIRED) 

# Android Logging
find_library(log-lib log)

# KL520 SDK, pthread, and USB (optional; uncomment and configure as needed)
# find_library(pthread-lib pthread)
# find_library(usb-lib usb-1.0)

# Consolidated link statement
target_link_libraries(kl520_ffi_lib
    # ${OpenCV_LIBS}
    ${log-lib}
    # ${pthread-lib}
    # ${usb-lib}
)
