cmake_minimum_required(VERSION 3.10)
set(CMAKE_VERBOSE_MAKEFILE on)

add_library(kl520_infer SHARED IMPORTED)
set_target_properties(kl520_infer PROPERTIES IMPORTED_LOCATION
    ${CMAKE_SOURCE_DIR}/src/main/jniLibs/${ANDROID_ABI}/libkl520_infer.so)

target_link_libraries(native-lib kl520_infer)
