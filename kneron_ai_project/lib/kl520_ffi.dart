import 'dart:ffi';
import 'dart:io';
import 'package:ffi/ffi.dart';

// Load shared library
final DynamicLibrary kl520Lib = Platform.isAndroid
    ? DynamicLibrary.open('libkl520_ffi_lib.so')
    : throw UnsupportedError('Only Android supported');

// --- C Struct Definitions (Mirroring C data structures) ---

// Inferred from kp_usb_jni.c [1-5] and kp_usb.c [7, 8]
// Assuming fixed sizes for char arrays based on common practice.
const int KpUsbMaxStringLength = 128; // Example size for port_path, firmware

@Packed(4) // Or appropriate alignment if known, 4-byte alignment is common
base class KpDeviceDescriptorC extends Struct {
  @Uint16()
  external int vendorId; // Maps to KpDeviceDescriptor.vendorId [14]
  @Uint16()
  external int productId; // Maps to KpDeviceDescriptor.productId [14]
  @Uint32() // kp_usb_speed_t (enum) often stored as int [14]
  external int linkSpeed;
  @Uint32() // Kneron-specific KN number, Uint32 in C [14]
  external int knNumber;
  @Bool()
  external bool isConnectable; // Maps to KpDeviceDescriptor.isConnectable [15]

  @Array(KpUsbMaxStringLength)
  external Array<Uint8> portPath; // Low-level USB port path [15]

  @Array(KpUsbMaxStringLength)
  external Array<Uint8> firmware; // Firmware description [15]
  @Uint32()
  external int portId; // Maps to UsbDevice.getDeviceId() [14]

  String get portPathDart => portPath.toString();
  String get firmwareDart => firmware.toString();
}

// Inferred from kp_usb_jni.c [1]
const int KpUsbMaxDevices = 20; // MAX_GROUP_DEVICE in kp_core.c [16, 17]

@Packed(4) // Or appropriate alignment
base class KpDevicesListC extends Struct {
  @Int32()
  external int numDev;
  @Array(KpUsbMaxDevices)
  external Array<KpDeviceDescriptorC> device; // Array of device descriptors
}

// Inferred from kp_usb.c [6-8]
// This structure holds the C-side representation of a connected Kneron USB device,
// including a pointer to the JNI handle that references Android USB objects.
@Packed(4) // Or appropriate alignment
base class KpUsbDeviceC extends Struct {
  // usb_device_handle_t* in C, which holds JNI references to Kotlin objects [18, 19]
  external Pointer<Void> usbHandle;
  external KpDeviceDescriptorC deviceDescriptor; // Embedded device descriptor
  @Uint16()
  external int fwSerial; // firmware serial from desc.bcdDevice [8]
  @Uint8()
  external int endpointCmdIn; // Kneron command IN endpoint [8]
  @Uint8()
  external int endpointCmdOut; // Kneron command OUT endpoint [8]
  @Uint8()
  external int endpointLogIn; // Kneron log IN endpoint (optional) [8]
  // Note: pthread_mutex_t are C-internal and not exposed via FFI
}

// Inferred kp_usb_control_t structure from kp_usb.c [20]
@Packed(4)
base class KpUsbControlC extends Struct {
  @Uint32()
  external int command;
  @Uint16()
  external int arg1;
  @Uint16()
  external int arg2;
}

// --- Typedefs and Lookups for USB functions (Updated with Structs) ---

// kp_devices_list_t *kp_usb_scan_devices() [21, 22]
typedef KpUsbScanDevicesC = Pointer<KpDevicesListC> Function();
typedef KpUsbScanDevicesDart = Pointer<KpDevicesListC> Function();
final KpUsbScanDevicesDart kpUsbScanDevices = kl520Lib
    .lookup<NativeFunction<KpUsbScanDevicesC>>('kp_usb_scan_devices')
    .asFunction();

// int kp_usb_connect_multiple_devices_v2(int num_dev, int port_id[], kp_usb_device_t *output_devs[], int try_count) [17, 23]
typedef KpUsbConnectMultipleDevicesV2C = Int32 Function(
    Int32 numDev,
    Pointer<Int32> portId,
    Pointer<Pointer<KpUsbDeviceC>> outputDevs, // Updated to use KpUsbDeviceC
    Int32 tryCount
);
typedef KpUsbConnectMultipleDevicesV2Dart = int Function(
    int numDev,
    Pointer<Int32> portId,
    Pointer<Pointer<KpUsbDeviceC>> outputDevs,
    int tryCount
);
final KpUsbConnectMultipleDevicesV2Dart kpUsbConnectMultipleDevicesV2 = kl520Lib
    .lookup<NativeFunction<KpUsbConnectMultipleDevicesV2C>>('kp_usb_connect_multiple_devices_v2')
    .asFunction();

// int kp_usb_disconnect_device(kp_usb_device_t *dev) [24, 25]
typedef KpUsbDisconnectDeviceC = Int32 Function(Pointer<KpUsbDeviceC> dev); // Updated
typedef KpUsbDisconnectDeviceDart = int Function(Pointer<KpUsbDeviceC> dev); // Updated
final KpUsbDisconnectDeviceDart kpUsbDisconnectDevice = kl520Lib
    .lookup<NativeFunction<KpUsbDisconnectDeviceC>>('kp_usb_disconnect_device')
    .asFunction();

// int kp_usb_disconnect_multiple_devices(int num_dev, kp_usb_device_t *devs[]) [24, 26]
typedef KpUsbDisconnectMultipleDevicesC = Int32 Function(Int32 numDev, Pointer<Pointer<KpUsbDeviceC>> devs); // Updated
typedef KpUsbDisconnectMultipleDevicesDart = int Function(int numDev, Pointer<Pointer<KpUsbDeviceC>> devs); // Updated
final KpUsbDisconnectMultipleDevicesDart kpUsbDisconnectMultipleDevices = kl520Lib
    .lookup<NativeFunction<KpUsbDisconnectMultipleDevicesC>>('kp_usb_disconnect_multiple_devices')
    .asFunction();

// kp_device_descriptor_t *kp_usb_get_device_descriptor(kp_usb_device_t *dev) [27, 28]
typedef KpUsbGetDeviceDescriptorC = Pointer<KpDeviceDescriptorC> Function(Pointer<KpUsbDeviceC> dev); // Updated
typedef KpUsbGetDeviceDescriptorDart = Pointer<KpDeviceDescriptorC> Function(Pointer<KpUsbDeviceC> dev); // Updated
final KpUsbGetDeviceDescriptorDart kpUsbGetDeviceDescriptor = kl520Lib
    .lookup<NativeFunction<KpUsbGetDeviceDescriptorC>>('kp_usb_get_device_descriptor')
    .asFunction();

// void kp_usb_flush_out_buffers(kp_usb_device_t *dev) [28, 29]
typedef KpUsbFlushOutBuffersC = Void Function(Pointer<KpUsbDeviceC> dev); // Updated
typedef KpUsbFlushOutBuffersDart = void Function(Pointer<KpUsbDeviceC> dev); // Updated
final KpUsbFlushOutBuffersDart kpUsbFlushOutBuffers = kl520Lib
    .lookup<NativeFunction<KpUsbFlushOutBuffersC>>('kp_usb_flush_out_buffers')
    .asFunction();

// int kp_usb_write_data(kp_usb_device_t *dev, void *buf, int len, int timeout) [29, 30]
typedef KpUsbWriteDataC = Int32 Function(Pointer<KpUsbDeviceC> dev, Pointer<Void> buf, Int32 len, Int32 timeout); // Updated
typedef KpUsbWriteDataDart = int Function(Pointer<KpUsbDeviceC> dev, Pointer<Void> buf, int len, int timeout); // Updated
final KpUsbWriteDataDart kpUsbWriteData = kl520Lib
    .lookup<NativeFunction<KpUsbWriteDataC>>('kp_usb_write_data')
    .asFunction();

// int kp_usb_read_data(kp_usb_device_t *dev, void *buf, int len, int timeout) [31, 32]
typedef KpUsbReadDataC = Int32 Function(Pointer<KpUsbDeviceC> dev, Pointer<Void> buf, Int32 len, Int32 timeout); // Updated
typedef KpUsbReadDataDart = int Function(Pointer<KpUsbDeviceC> dev, Pointer<Void> buf, int len, int timeout); // Updated
final KpUsbReadDataDart kpUsbReadData = kl520Lib
    .lookup<NativeFunction<KpUsbReadDataC>>('kp_usb_read_data')
    .asFunction();

// int kp_usb_endpoint_write_data(kp_usb_device_t *dev, int endpoint, void *buf, int len, int timeout) [33, 34]
typedef KpUsbEndpointWriteDataC = Int32 Function(Pointer<KpUsbDeviceC> dev, Int32 endpoint, Pointer<Void> buf, Int32 len, Int32 timeout); // Updated
typedef KpUsbEndpointWriteDataDart = int Function(Pointer<KpUsbDeviceC> dev, int endpoint, Pointer<Void> buf, int len, int timeout); // Updated
final KpUsbEndpointWriteDataDart kpUsbEndpointWriteData = kl520Lib
    .lookup<NativeFunction<KpUsbEndpointWriteDataC>>('kp_usb_endpoint_write_data')
    .asFunction();

// int kp_usb_endpoint_read_data(kp_usb_device_t *dev, int endpoint, void *buf, int len, int timeout) [34, 35]
typedef KpUsbEndpointReadDataC = Int32 Function(Pointer<KpUsbDeviceC> dev, Int32 endpoint, Pointer<Void> buf, Int32 len, Int32 timeout); // Updated
typedef KpUsbEndpointReadDataDart = int Function(Pointer<KpUsbDeviceC> dev, int endpoint, Pointer<Void> buf, int len, int timeout); // Updated
final KpUsbEndpointReadDataDart kpUsbEndpointReadData = kl520Lib
    .lookup<NativeFunction<KpUsbEndpointReadDataC>>('kp_usb_endpoint_read_data')
    .asFunction();

// // int kp_usb_control(kp_usb_device_t *dev, kp_usb_control_t *control_request, int timeout) [20, 35]
// typedef KpUsbControlC = Int32 Function(Pointer<KpUsbDeviceC> dev, Pointer<KpUsbControlC> controlRequest, Int32 timeout); // Updated
// typedef KpUsbControlDart = int Function(Pointer<KpUsbDeviceC> dev, Pointer<KpUsbControlC> controlRequest, int timeout); // Updated
// final KpUsbControlDart kpUsbControl = kl520Lib
//     .lookup<NativeFunction<KpUsbControlC>>('kp_usb_control')
//     .asFunction();

// int kp_usb_read_firmware_log(kp_usb_device_t *dev, void *buf, int len, int timeout) [36, 37]
typedef KpUsbReadFirmwareLogC = Int32 Function(Pointer<KpUsbDeviceC> dev, Pointer<Void> buf, Int32 len, Int32 timeout); // Updated
typedef KpUsbReadFirmwareLogDart = int Function(Pointer<KpUsbDeviceC> dev, Pointer<Void> buf, int len, int timeout); // Updated
final KpUsbReadFirmwareLogDart kpUsbReadFirmwareLog = kl520Lib
    .lookup<NativeFunction<KpUsbReadFirmwareLogC>>('kp_usb_read_firmware_log')
    .asFunction();

// --- Higher-level C wrapper functions (conditional lookups) ---
// These functions are generally intended to be replaced by Dart/Kotlin logic
// as part of the C-to-Dart migration but are kept here as they exist in the source. [38-43]

// int kp_scan_devices() - alias for kp_usb_scan_devices [44]
typedef ScanDevicesC = Pointer<KpDevicesListC> Function(); // Updated to return KpDevicesListC
typedef ScanDevicesDart = Pointer<KpDevicesListC> Function(); // Updated to return KpDevicesListC
final ScanDevicesDart? scanDevices = kl520Lib.providesSymbol('kp_scan_devices')
    ? kl520Lib.lookup<NativeFunction<ScanDevicesC>>('kp_scan_devices').asFunction()
    : null;

// int ffi_kp_connect_device(); (placeholder/higher-level)
typedef FfiKpConnectDeviceC = Int32 Function();
typedef FfiKpConnectDeviceDart = int Function();
final FfiKpConnectDeviceDart? ffiKpConnectDevice = kl520Lib.providesSymbol('ffi_kp_connect_device')
    ? kl520Lib.lookup<NativeFunction<FfiKpConnectDeviceC>>('ffi_kp_connect_device').asFunction()
    : null;

// int ffi_kp_load_firmware(const char *scpu_path, const char *ncpu_path); (placeholder/higher-level)
typedef FfiKpLoadFirmwareC = Int32 Function(Pointer<Utf8> scpuPath, Pointer<Utf8> ncpuPath);
typedef FfiKpLoadFirmwareDart = int Function(Pointer<Utf8> scpuPath, Pointer<Utf8> ncpuPath);
final FfiKpLoadFirmwareDart? ffiKpLoadFirmware = kl520Lib.providesSymbol('ffi_kp_load_firmware')
    ? kl520Lib.lookup<NativeFunction<FfiKpLoadFirmwareC>>('ffi_kp_load_firmware').asFunction()
    : null;

// int ffi_kp_load_model(const char *model_path); (placeholder/higher-level)
typedef FfiKpLoadModelC = Int32 Function(Pointer<Utf8> modelPath);
typedef FfiKpLoadModelDart = int Function(Pointer<Utf8> modelPath);
final FfiKpLoadModelDart? ffiKpLoadModel = kl520Lib.providesSymbol('ffi_kp_load_model')
    ? kl520Lib.lookup<NativeFunction<FfiKpLoadModelC>>('ffi_kp_load_model').asFunction()
    : null;

// int ffi_kp_configure_inference(bool enable_frame_drop); (placeholder/higher-level)
typedef FfiKpConfigureInferenceC = Int32 Function(Int32 enableFrameDrop);
typedef FfiKpConfigureInferenceDart = int Function(int enableFrameDrop);
final FfiKpConfigureInferenceDart? ffiKpConfigureInference = kl520Lib.providesSymbol('ffi_kp_configure_inference')
    ? kl520Lib.lookup<NativeFunction<FfiKpConfigureInferenceC>>('ffi_kp_configure_inference').asFunction()
    : null;

// int ffi_kp_disconnect_device(); (placeholder/higher-level)
typedef FfiKpDisconnectDeviceC = Int32 Function();
typedef FfiKpDisconnectDeviceDart = int Function();
final FfiKpDisconnectDeviceDart? ffiKpDisconnectDevice = kl520Lib.providesSymbol('ffi_kp_disconnect_device')
    ? kl520Lib.lookup<NativeFunction<FfiKpDisconnectDeviceC>>('ffi_kp_disconnect_device').asFunction()
    : null;

// int ffi_kp_send_image(uint8_t *buffer, int width, int height); (placeholder/higher-level)
typedef FfiKpSendImageC = Int32 Function(Pointer<Uint8> buffer, Int32 width, Int32 height);
typedef FfiKpSendImageDart = int Function(Pointer<Uint8> buffer, int width, int height);
final FfiKpSendImageDart? ffiKpSendImage = kl520Lib.providesSymbol('ffi_kp_send_image')
    ? kl520Lib.lookup<NativeFunction<FfiKpSendImageC>>('ffi_kp_send_image').asFunction()
    : null;

// int ffi_kp_receive_result(float *out_result_buffer, int max_buffer_size); (placeholder/higher-level)
typedef FfiKpReceiveResultC = Int32 Function(Pointer<Float> outResultBuffer, Int32 maxBufferSize);
typedef FfiKpReceiveResultDart = int Function(Pointer<Float> outResultBuffer, int maxBufferSize);
final FfiKpReceiveResultDart? ffiKpReceiveResult = kl520Lib.providesSymbol('ffi_kp_receive_result')
    ? kl520Lib.lookup<NativeFunction<FfiKpReceiveResultC>>('ffi_kp_receive_result').asFunction()
    : null;

// int ffi_kp_get_model_resolution(int *width, int *height); (placeholder/higher-level)
typedef FfiKpGetModelResolutionC = Int32 Function(Pointer<Int32> width, Pointer<Int32> height);
typedef FfiKpGetModelResolutionDart = int Function(Pointer<Int32> width, Pointer<Int32> height);
final FfiKpGetModelResolutionDart? ffiKpGetModelResolution = kl520Lib.providesSymbol('ffi_kp_get_model_resolution')
    ? kl520Lib.lookup<NativeFunction<FfiKpGetModelResolutionC>>('ffi_kp_get_model_resolution').asFunction()
    : null;
    
// Function binding for usb_jni_cleanup (to free C-side memory) [13]
typedef UsbJniCleanupC = Void Function();
typedef UsbJniCleanupDart = void Function();

final UsbJniCleanupDart usbJniCleanup = kl520Lib
    .lookup<NativeFunction<UsbJniCleanupC>>('usb_jni_cleanup')
    .asFunction();