import 'dart:ffi';
import 'dart:io';
import 'package:ffi/ffi.dart';

// Load shared library
final DynamicLibrary kl520Lib = Platform.isAndroid
    ? DynamicLibrary.open('libkl520_ffi_lib.so')
    : throw UnsupportedError('Only Android supported');

// Typedefs and Lookups for USB functions

// kp_devices_list_t *kp_usb_scan_devices()
typedef KpUsbScanDevicesC = Pointer<Void> Function();
typedef KpUsbScanDevicesDart = Pointer<Void> Function();

final KpUsbScanDevicesDart kpUsbScanDevices = kl520Lib
    .lookup<NativeFunction<KpUsbScanDevicesC>>('kp_usb_scan_devices')
    .asFunction<KpUsbScanDevicesDart>();

// int kp_usb_connect_multiple_devices_v2(int num_dev, int port_id[], kp_usb_device_t *output_devs[], int try_count)
typedef KpUsbConnectMultipleDevicesV2C = Int32 Function(
    Int32 numDev, 
    Pointer<Int32> portId, 
    Pointer<Pointer<Void>> outputDevs, 
    Int32 tryCount
);
typedef KpUsbConnectMultipleDevicesV2Dart = int Function(
    int numDev, 
    Pointer<Int32> portId, 
    Pointer<Pointer<Void>> outputDevs, 
    int tryCount
);

final KpUsbConnectMultipleDevicesV2Dart kpUsbConnectMultipleDevicesV2 = kl520Lib
    .lookup<NativeFunction<KpUsbConnectMultipleDevicesV2C>>('kp_usb_connect_multiple_devices_v2')
    .asFunction<KpUsbConnectMultipleDevicesV2Dart>();

// int kp_usb_disconnect_device(kp_usb_device_t *dev)
typedef KpUsbDisconnectDeviceC = Int32 Function(Pointer<Void> dev);
typedef KpUsbDisconnectDeviceDart = int Function(Pointer<Void> dev);

final KpUsbDisconnectDeviceDart kpUsbDisconnectDevice = kl520Lib
    .lookup<NativeFunction<KpUsbDisconnectDeviceC>>('kp_usb_disconnect_device')
    .asFunction<KpUsbDisconnectDeviceDart>();

// int kp_usb_disconnect_multiple_devices(int num_dev, kp_usb_device_t *devs[])
typedef KpUsbDisconnectMultipleDevicesC = Int32 Function(Int32 numDev, Pointer<Pointer<Void>> devs);
typedef KpUsbDisconnectMultipleDevicesDart = int Function(int numDev, Pointer<Pointer<Void>> devs);

final KpUsbDisconnectMultipleDevicesDart kpUsbDisconnectMultipleDevices = kl520Lib
    .lookup<NativeFunction<KpUsbDisconnectMultipleDevicesC>>('kp_usb_disconnect_multiple_devices')
    .asFunction<KpUsbDisconnectMultipleDevicesDart>();

// kp_device_descriptor_t *kp_usb_get_device_descriptor(kp_usb_device_t *dev)
typedef KpUsbGetDeviceDescriptorC = Pointer<Void> Function(Pointer<Void> dev);
typedef KpUsbGetDeviceDescriptorDart = Pointer<Void> Function(Pointer<Void> dev);

final KpUsbGetDeviceDescriptorDart kpUsbGetDeviceDescriptor = kl520Lib
    .lookup<NativeFunction<KpUsbGetDeviceDescriptorC>>('kp_usb_get_device_descriptor')
    .asFunction<KpUsbGetDeviceDescriptorDart>();

// void kp_usb_flush_out_buffers(kp_usb_device_t *dev)
typedef KpUsbFlushOutBuffersC = Void Function(Pointer<Void> dev);
typedef KpUsbFlushOutBuffersDart = void Function(Pointer<Void> dev);

final KpUsbFlushOutBuffersDart kpUsbFlushOutBuffers = kl520Lib
    .lookup<NativeFunction<KpUsbFlushOutBuffersC>>('kp_usb_flush_out_buffers')
    .asFunction<KpUsbFlushOutBuffersDart>();

// int kp_usb_write_data(kp_usb_device_t *dev, void *buf, int len, int timeout)
typedef KpUsbWriteDataC = Int32 Function(Pointer<Void> dev, Pointer<Void> buf, Int32 len, Int32 timeout);
typedef KpUsbWriteDataDart = int Function(Pointer<Void> dev, Pointer<Void> buf, int len, int timeout);

final KpUsbWriteDataDart kpUsbWriteData = kl520Lib
    .lookup<NativeFunction<KpUsbWriteDataC>>('kp_usb_write_data')
    .asFunction<KpUsbWriteDataDart>();

// int kp_usb_read_data(kp_usb_device_t *dev, void *buf, int len, int timeout)
typedef KpUsbReadDataC = Int32 Function(Pointer<Void> dev, Pointer<Void> buf, Int32 len, Int32 timeout);
typedef KpUsbReadDataDart = int Function(Pointer<Void> dev, Pointer<Void> buf, int len, int timeout);

final KpUsbReadDataDart kpUsbReadData = kl520Lib
    .lookup<NativeFunction<KpUsbReadDataC>>('kp_usb_read_data')
    .asFunction<KpUsbReadDataDart>();

// int kp_usb_endpoint_write_data(kp_usb_device_t *dev, int endpoint, void *buf, int len, int timeout)
typedef KpUsbEndpointWriteDataC = Int32 Function(Pointer<Void> dev, Int32 endpoint, Pointer<Void> buf, Int32 len, Int32 timeout);
typedef KpUsbEndpointWriteDataDart = int Function(Pointer<Void> dev, int endpoint, Pointer<Void> buf, int len, int timeout);

final KpUsbEndpointWriteDataDart kpUsbEndpointWriteData = kl520Lib
    .lookup<NativeFunction<KpUsbEndpointWriteDataC>>('kp_usb_endpoint_write_data')
    .asFunction<KpUsbEndpointWriteDataDart>();

// int kp_usb_endpoint_read_data(kp_usb_device_t *dev, int endpoint, void *buf, int len, int timeout)
typedef KpUsbEndpointReadDataC = Int32 Function(Pointer<Void> dev, Int32 endpoint, Pointer<Void> buf, Int32 len, Int32 timeout);
typedef KpUsbEndpointReadDataDart = int Function(Pointer<Void> dev, int endpoint, Pointer<Void> buf, int len, int timeout);

final KpUsbEndpointReadDataDart kpUsbEndpointReadData = kl520Lib
    .lookup<NativeFunction<KpUsbEndpointReadDataC>>('kp_usb_endpoint_read_data')
    .asFunction<KpUsbEndpointReadDataDart>();

// int kp_usb_control(kp_usb_device_t *dev, kp_usb_control_t *control_request, int timeout)
typedef KpUsbControlC = Int32 Function(Pointer<Void> dev, Pointer<Void> controlRequest, Int32 timeout);
typedef KpUsbControlDart = int Function(Pointer<Void> dev, Pointer<Void> controlRequest, int timeout);

final KpUsbControlDart kpUsbControl = kl520Lib
    .lookup<NativeFunction<KpUsbControlC>>('kp_usb_control')
    .asFunction<KpUsbControlDart>();

// int kp_usb_read_firmware_log(kp_usb_device_t *dev, void *buf, int len, int timeout)
typedef KpUsbReadFirmwareLogC = Int32 Function(Pointer<Void> dev, Pointer<Void> buf, Int32 len, Int32 timeout);
typedef KpUsbReadFirmwareLogDart = int Function(Pointer<Void> dev, Pointer<Void> buf, int len, int timeout);

final KpUsbReadFirmwareLogDart kpUsbReadFirmwareLog = kl520Lib
    .lookup<NativeFunction<KpUsbReadFirmwareLogC>>('kp_usb_read_firmware_log')
    .asFunction<KpUsbReadFirmwareLogDart>();

// If you still have the higher-level wrapper functions, keep them as well:

// int scan_devices(int argc, char *argv[]);
typedef ScanDevicesC = Int32 Function(Int32 argc, Pointer<Pointer<Utf8>> argv);
typedef ScanDevicesDart = int Function(int argc, Pointer<Pointer<Utf8>> argv);

final ScanDevicesDart? scanDevices = kl520Lib.providesSymbol('kp_scan_devices') 
    ? kl520Lib.lookup<NativeFunction<ScanDevicesC>>('kp_scan_devices').asFunction<ScanDevicesDart>()
    : null;

// int ffi_kp_connect_device();
typedef FfiKpConnectDeviceC = Int32 Function();
typedef FfiKpConnectDeviceDart = int Function();

final FfiKpConnectDeviceDart? ffiKpConnectDevice = kl520Lib.providesSymbol('ffi_kp_connect_device')
    ? kl520Lib.lookup<NativeFunction<FfiKpConnectDeviceC>>('ffi_kp_connect_device').asFunction<FfiKpConnectDeviceDart>()
    : null;

// int ffi_kp_load_firmware(const char *scpu_path, const char *ncpu_path);
typedef FfiKpLoadFirmwareC = Int32 Function(Pointer<Utf8> scpuPath, Pointer<Utf8> ncpuPath);
typedef FfiKpLoadFirmwareDart = int Function(Pointer<Utf8> scpuPath, Pointer<Utf8> ncpuPath);

final FfiKpLoadFirmwareDart? ffiKpLoadFirmware = kl520Lib.providesSymbol('ffi_kp_load_firmware')
    ? kl520Lib.lookup<NativeFunction<FfiKpLoadFirmwareC>>('ffi_kp_load_firmware').asFunction<FfiKpLoadFirmwareDart>()
    : null;

// int ffi_kp_load_model(const char *model_path);
typedef FfiKpLoadModelC = Int32 Function(Pointer<Utf8> modelPath);
typedef FfiKpLoadModelDart = int Function(Pointer<Utf8> modelPath);

final FfiKpLoadModelDart? ffiKpLoadModel = kl520Lib.providesSymbol('ffi_kp_load_model')
    ? kl520Lib.lookup<NativeFunction<FfiKpLoadModelC>>('ffi_kp_load_model').asFunction<FfiKpLoadModelDart>()
    : null;

// int ffi_kp_configure_inference(bool enable_frame_drop);
typedef FfiKpConfigureInferenceC = Int32 Function(Int32 enableFrameDrop);
typedef FfiKpConfigureInferenceDart = int Function(int enableFrameDrop);

final FfiKpConfigureInferenceDart? ffiKpConfigureInference = kl520Lib.providesSymbol('ffi_kp_configure_inference')
    ? kl520Lib.lookup<NativeFunction<FfiKpConfigureInferenceC>>('ffi_kp_configure_inference').asFunction<FfiKpConfigureInferenceDart>()
    : null;

// int ffi_kp_disconnect_device();
typedef FfiKpDisconnectDeviceC = Int32 Function();
typedef FfiKpDisconnectDeviceDart = int Function();

final FfiKpDisconnectDeviceDart? ffiKpDisconnectDevice = kl520Lib.providesSymbol('ffi_kp_disconnect_device')
    ? kl520Lib.lookup<NativeFunction<FfiKpDisconnectDeviceC>>('ffi_kp_disconnect_device').asFunction<FfiKpDisconnectDeviceDart>()
    : null;

// int ffi_kp_send_image(uint8_t *buffer, int width, int height);
typedef FfiKpSendImageC = Int32 Function(Pointer<Uint8> buffer, Int32 width, Int32 height);
typedef FfiKpSendImageDart = int Function(Pointer<Uint8> buffer, int width, int height);

final FfiKpSendImageDart? ffiKpSendImage = kl520Lib.providesSymbol('ffi_kp_send_image')
    ? kl520Lib.lookup<NativeFunction<FfiKpSendImageC>>('ffi_kp_send_image').asFunction<FfiKpSendImageDart>()
    : null;

// int ffi_kp_receive_result(float *out_result_buffer, int max_buffer_size);
typedef FfiKpReceiveResultC = Int32 Function(Pointer<Float> outResultBuffer, Int32 maxBufferSize);
typedef FfiKpReceiveResultDart = int Function(Pointer<Float> outResultBuffer, int maxBufferSize);

final FfiKpReceiveResultDart? ffiKpReceiveResult = kl520Lib.providesSymbol('ffi_kp_receive_result')
    ? kl520Lib.lookup<NativeFunction<FfiKpReceiveResultC>>('ffi_kp_receive_result').asFunction<FfiKpReceiveResultDart>()
    : null;

// int ffi_kp_get_model_resolution(int *width, int *height);
typedef FfiKpGetModelResolutionC = Int32 Function(Pointer<Int32> width, Pointer<Int32> height);
typedef FfiKpGetModelResolutionDart = int Function(Pointer<Int32> width, Pointer<Int32> height);

final FfiKpGetModelResolutionDart? ffiKpGetModelResolution = kl520Lib.providesSymbol('ffi_kp_get_model_resolution')
    ? kl520Lib.lookup<NativeFunction<FfiKpGetModelResolutionC>>('ffi_kp_get_model_resolution').asFunction<FfiKpGetModelResolutionDart>()
    : null;