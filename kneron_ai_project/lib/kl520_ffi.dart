import 'dart:ffi';
import 'dart:io';
import 'package:ffi/ffi.dart';

// Load shared library
final DynamicLibrary kl520Lib = Platform.isAndroid
    ? DynamicLibrary.open('libkl520_ffi_lib.so')
    : throw UnsupportedError('Only Android supported');

// Typedefs and Lookups

// int scan_devices(int argc, char *argv[]);
typedef ScanDevicesC = Int32 Function(Int32 argc, Pointer<Pointer<Utf8>> argv);
typedef ScanDevicesDart = int Function(int argc, Pointer<Pointer<Utf8>> argv);

final ScanDevicesDart scanDevices = kl520Lib
    .lookup<NativeFunction<ScanDevicesC>>('scan_devices')
    .asFunction<ScanDevicesDart>();

// int ffi_kp_connect_device();
typedef FfiKpConnectDeviceC = Int32 Function();
typedef FfiKpConnectDeviceDart = int Function();

final FfiKpConnectDeviceDart ffiKpConnectDevice = kl520Lib
    .lookup<NativeFunction<FfiKpConnectDeviceC>>('ffi_kp_connect_device')
    .asFunction<FfiKpConnectDeviceDart>();

// int ffi_kp_load_firmware(const char *scpu_path, const char *ncpu_path);
typedef FfiKpLoadFirmwareC = Int32 Function(Pointer<Utf8> scpuPath, Pointer<Utf8> ncpuPath);
typedef FfiKpLoadFirmwareDart = int Function(Pointer<Utf8> scpuPath, Pointer<Utf8> ncpuPath);

final FfiKpLoadFirmwareDart ffiKpLoadFirmware = kl520Lib
    .lookup<NativeFunction<FfiKpLoadFirmwareC>>('ffi_kp_load_firmware')
    .asFunction<FfiKpLoadFirmwareDart>();

// int ffi_kp_load_model(const char *model_path);
typedef FfiKpLoadModelC = Int32 Function(Pointer<Utf8> modelPath);
typedef FfiKpLoadModelDart = int Function(Pointer<Utf8> modelPath);

final FfiKpLoadModelDart ffiKpLoadModel = kl520Lib
    .lookup<NativeFunction<FfiKpLoadModelC>>('ffi_kp_load_model')
    .asFunction<FfiKpLoadModelDart>();

// int ffi_kp_configure_inference(bool enable_frame_drop);
typedef FfiKpConfigureInferenceC = Int32 Function(Int32 enableFrameDrop);
typedef FfiKpConfigureInferenceDart = int Function(int enableFrameDrop);

final FfiKpConfigureInferenceDart ffiKpConfigureInference = kl520Lib
    .lookup<NativeFunction<FfiKpConfigureInferenceC>>('ffi_kp_configure_inference')
    .asFunction<FfiKpConfigureInferenceDart>();

// int ffi_kp_disconnect_device();
typedef FfiKpDisconnectDeviceC = Int32 Function();
typedef FfiKpDisconnectDeviceDart = int Function();

final FfiKpDisconnectDeviceDart ffiKpDisconnectDevice = kl520Lib
    .lookup<NativeFunction<FfiKpDisconnectDeviceC>>('ffi_kp_disconnect_device')
    .asFunction<FfiKpDisconnectDeviceDart>();

// int ffi_kp_send_image(uint8_t *buffer, int width, int height);
typedef FfiKpSendImageC = Int32 Function(Pointer<Uint8> buffer, Int32 width, Int32 height);
typedef FfiKpSendImageDart = int Function(Pointer<Uint8> buffer, int width, int height);

final FfiKpSendImageDart ffiKpSendImage = kl520Lib
    .lookup<NativeFunction<FfiKpSendImageC>>('ffi_kp_send_image')
    .asFunction<FfiKpSendImageDart>();

// int ffi_kp_receive_result(float *out_result_buffer, int max_buffer_size);
typedef FfiKpReceiveResultC = Int32 Function(Pointer<Float> outResultBuffer, Int32 maxBufferSize);
typedef FfiKpReceiveResultDart = int Function(Pointer<Float> outResultBuffer, int maxBufferSize);

final FfiKpReceiveResultDart ffiKpReceiveResult = kl520Lib
    .lookup<NativeFunction<FfiKpReceiveResultC>>('ffi_kp_receive_result')
    .asFunction<FfiKpReceiveResultDart>();

// int ffi_kp_get_model_resolution(int *width, int *height);
typedef FfiKpGetModelResolutionC = Int32 Function(Pointer<Int32> width, Pointer<Int32> height);
typedef FfiKpGetModelResolutionDart = int Function(Pointer<Int32> width, Pointer<Int32> height);

final FfiKpGetModelResolutionDart ffiKpGetModelResolution = kl520Lib
    .lookup<NativeFunction<FfiKpGetModelResolutionC>>('ffi_kp_get_model_resolution')
    .asFunction<FfiKpGetModelResolutionDart>();
