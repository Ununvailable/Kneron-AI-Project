import 'dart:ffi';
import 'dart:io';

// C function signature: int kl520_run_inference(uint8_t* input_buf, int len);
typedef Kl520RunInferenceC = Int32 Function(Pointer<Uint8>, Int32); // Native C
typedef Kl520RunInferenceDart = int Function(Pointer<Uint8>, int);  // Dart
typedef Kl520InitializeC = Int32 Function();
typedef Kl520InitializeDart = int Function();

final DynamicLibrary kl520Lib = Platform.isAndroid
    ? DynamicLibrary.open('libkl520_infer.so')
    : throw UnsupportedError('Only Android supported');

final Kl520InitializeDart kl520Initialize = kl520Lib
    .lookup<NativeFunction<Kl520InitializeC>>('kl520_initialize')
    .asFunction<Kl520InitializeDart>();

final Kl520RunInferenceDart kl520RunInference = kl520Lib
    .lookup<NativeFunction<Kl520RunInferenceC>>('kl520_run_inference')
    .asFunction<Kl520RunInferenceDart>(); 