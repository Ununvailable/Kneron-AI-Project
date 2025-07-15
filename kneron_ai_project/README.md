# Kneron AI Android Application Project

This project is a Flutter-based Android application integrating with Kneron KL520 AI inference devices.  
The application utilizes a native C++ shared library accessed via Dart FFI for communication with Kneron hardware.

---

## Project Overview

As of now, this project consists of the following layers:

- **Flutter/Dart UI Layer**  
  Provides user interface, user interactions, and application logic in Dart.

- **Dart FFI Layer**  
  - File: `kl520_ffi.dart`  
  - Declares native function signatures and loads the shared library.  
  - Calls native C++ functions via Dart FFI.

- **Native C++ Shared Library**  
  - Output: `libkl520_ffi_lib.so`  
  - Built from: `kl520_android_application.cpp` and supporting C/C++ source files.  
  - Implements device scanning, firmware loading, model loading, inference control, and image processing.  
  - Integrates with the Kneron PLUS SDK and requires `libusb` for USB device access.

- **Kneron PLUS SDK and Dependencies**  
  - Provides core device management and AI inference APIs (via native SDK sources).  
  - Requires `libusb` for device communication on Android.  
  - Integration may involve additional JNI for Android USB permission handling.

---

## Getting Started with Flutter

Reference used:
- [Write your first Flutter app](https://docs.flutter.dev/get-started/codelab)
- [Cookbook: Useful Flutter samples](https://docs.flutter.dev/cookbook)
- [Flutter API Reference and Documentation](https://docs.flutter.dev/)

---

## Build and Integration Plan

- The native library (`libkl520_ffi_lib.so`) must be built for Android using **CMake** with NDK.
- Kneron SDK sources and `libusb` must be correctly included and linked in the build.
- Ensure USB permission handling on Android is implemented either via Dart/Java layer or JNI.
