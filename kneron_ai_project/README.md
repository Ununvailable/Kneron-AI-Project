# kneron_ai_project

A new Flutter project.

## Getting Started

This project is a starting point for a Flutter application.

A few resources to get you started if this is your first Flutter project:

- [Lab: Write your first Flutter app](https://docs.flutter.dev/get-started/codelab)
- [Cookbook: Useful Flutter samples](https://docs.flutter.dev/cookbook)

For help getting started with Flutter development, view the
[online documentation](https://docs.flutter.dev/), which offers tutorials,
samples, guidance on mobile development, and a full API reference.

## Architectural Overview

[ Dart UI Layer ]
   ↓
main.dart 
   ↓ (calls FFI functions)
kl520_ffi.dart  
   ↓ (binds to native C++ functions)
libkl520_ffi_lib.so 
   ↑ (built from kl520_android_application.cpp)
[ Native C++ Library with KL520 logic ]
