cmake_minimum_required(VERSION 3.6)

project(kl520_ffi_lib)

# Set C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# === Header Search Paths ===
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include             # Your project headers
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/include  # External headers (if any)
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

# === External Library Link Example (Optional) ===

# OpenCV (only if needed)
# find_package(OpenCV REQUIRED)
# include_directories(${OpenCV_INCLUDE_DIRS})
# target_link_libraries(kl520_ffi_lib ${OpenCV_LIBS})

# KL520 SDK (edit as needed)
# include_directories(/absolute/path/to/kneron_sdk/include)
# target_link_libraries(kl520_ffi_lib /absolute/path/to/kneron_sdk/lib/libkplus.so)

# Android Logging (recommended)
find_library(log-lib log)
target_link_libraries(kl520_ffi_lib ${log-lib})

# Pthread (optional on Android)
# find_library(pthread-lib pthread)
# target_link_libraries(kl520_ffi_lib ${pthread-lib})

# USB (if needed, rare for Android USB host)
# find_library(usb-lib usb-1.0)
# target_link_libraries(kl520_ffi_lib ${usb-lib})
