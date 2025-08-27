import 'dart:ffi';
import 'dart:convert';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:ffi/ffi.dart'; // Import ffi for String conversions
import 'kl520_ffi.dart';

// Define a MethodChannel for USB initialization
const platform = MethodChannel('kl520_usb_plugin/usb');

Future<void> initializeUsbBridge() async { // Changed return type to Future<void>
  try {
    final result = await platform.invokeMethod('initializeUsb');
    debugPrint("UsbHostBridge initialized with result: $result");
  } on PlatformException catch (e) {
    debugPrint("Failed to initialize UsbHostBridge: ${e.message}");
  }
}

String arrayToDartString(Array<Uint8> arr, arrLength) {
  final bytes = <int>[];
  for (var i = 0; i < arrLength; i++) {
    final v = arr[i];
    if (v == 0) break;
    bytes.add(v);
  }
  return utf8.decode(bytes);
}

void main() {
  runApp(const MaterialApp(home: KL520DeviceApp()));
}

class KL520DeviceApp extends StatefulWidget {
  const KL520DeviceApp({super.key});

  @override
  State<KL520DeviceApp> createState() => _KL520DeviceAppState();
}

class _KL520DeviceAppState extends State<KL520DeviceApp> {
  String _scanResult = "No scan performed yet";
  bool _isScanning = false;
  List<String> _deviceInfo = []; // Changed to List<String> for clarity

  @override
  void initState() {
    super.initState();
    initializeUsbBridge(); // Ensure JNI is initialized early
  }

  Future<void> _scanDevices() async {
    setState(() {
      _isScanning = true;
      _scanResult = "Scanning for KL520 devices...";
      _deviceInfo.clear();
    });

    try {
      // **Call the USB scan function from FFI**
      final Pointer<KpDevicesListC> devicesListPtr = kpUsbScanDevices().cast<KpDevicesListC>();

      if (devicesListPtr != nullptr) {
        final KpDevicesListC devicesList = devicesListPtr.ref;
        final int numDevices = devicesList.numDev;

        if (numDevices > 0) {
          setState(() {
            _scanResult = "Found **$numDevices** Kneron devices!";
            _deviceInfo.add("Device scan successful. Found $numDevices device(s).");
            // Iterate through the found devices and extract details
            for (int i = 0; i < numDevices; i++) {
              final KpDeviceDescriptorC device = devicesList.device[i];

              // Convert C-style fixed-size char arrays to Dart Strings
              // final String portPath = device.portPath.cast<Utf8>().toDartString();
              // final String firmware = device.firmware.cast<Utf8>().toDartString();
              final firmware = arrayToDartString(device.firmware, 30);
              final portPath = arrayToDartString(device.portPath, 20);


              _deviceInfo.add(
                "**Device ${i + 1}:**\n"
                "  Port ID: ${device.portId}\n"
                "  Vendor ID: 0x${device.vendorId.toRadixString(16)}\n"
                "  Product ID: 0x${device.productId.toRadixString(16)}\n"
                "  Link Speed: ${device.linkSpeed}\n"
                "  KN Number: 0x${device.knNumber.toRadixString(16)}\n"
                "  Connectable: ${device.isConnectable}\n"
                "  Port Path: $portPath\n"
                "  Firmware: $firmware"
              );
            }
          });
        } else {
          setState(() {
            _scanResult = "No Kneron devices found";
            _deviceInfo.add("No devices detected.");
            _deviceInfo.add("Check USB connections and device status.");
          });
        }
      } else {
        setState(() {
          _scanResult = "Error: Failed to retrieve device list pointer.";
          _deviceInfo.add("The device list pointer was null. This might indicate an FFI error or no devices were found by the native side.");
        });
      }
    } catch (e) {
      setState(() {
        _scanResult = "Error during scan: **$e**";
        _deviceInfo.add("Scan failed with error:");
        _deviceInfo.add(e.toString());
      });
    } finally {
      // **Clean up C-side memory after scan, if applicable.**
      // The `usb_jni_scan_devices` function in `kp_usb_jni.c` uses a static global buffer `g_kdev_list` [4, 23].
      // This buffer is freed by `usb_jni_cleanup` [13]. Calling `usbJniCleanup()` from Dart ensures
      // this memory is released.
      usbJniCleanup();
      setState(() {
        _isScanning = false;
      });
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text("KL520 Device Scanner"),
        backgroundColor: Colors.blue.shade700,
        foregroundColor: Colors.white,
      ),
      body: Padding(
        padding: const EdgeInsets.all(16.0),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.stretch,
          children: [
            ElevatedButton.icon(
              onPressed: _isScanning ? null : _scanDevices,
              icon: _isScanning
                  ? const SizedBox(
                      width: 20,
                      height: 20,
                      child: CircularProgressIndicator(strokeWidth: 2),
                    )
                  : const Icon(Icons.search),
              label: Text(_isScanning ? "Scanning..." : "Scan for KL520 Devices"),
              style: ElevatedButton.styleFrom(
                backgroundColor: Colors.blue.shade600,
                foregroundColor: Colors.white,
                padding: const EdgeInsets.symmetric(vertical: 16),
              ),
            ),
            const SizedBox(height: 24),
            Card(
              child: Padding(
                padding: const EdgeInsets.all(16.0),
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    const Text(
                      "Scan Results:",
                      style: TextStyle(
                        fontSize: 18,
                        fontWeight: FontWeight.bold,
                      ),
                    ),
                    const SizedBox(height: 8),
                    Text(
                      _scanResult,
                      style: TextStyle(
                        fontSize: 16,
                        color: _scanResult.contains("Error")
                            ? Colors.red.shade700
                            : _scanResult.contains("Found")
                                ? Colors.green.shade700
                                : Colors.black87,
                      ),
                    ),
                  ],
                ),
              ),
            ),
            const SizedBox(height: 16),
            if (_deviceInfo.isNotEmpty) ...[
              const Text(
                "Device Information:",
                style: TextStyle(
                  fontSize: 18,
                  fontWeight: FontWeight.bold,
                ),
              ),
              const SizedBox(height: 8),
              Expanded(
                child: Card(
                  child: ListView.builder(
                    padding: const EdgeInsets.all(16),
                    itemCount: _deviceInfo.length,
                    itemBuilder: (context, index) {
                      return Padding(
                        padding: const EdgeInsets.symmetric(vertical: 4),
                        child: Row(
                          crossAxisAlignment: CrossAxisAlignment.start,
                          children: [
                            Icon(
                              Icons.info_outline,
                              size: 16,
                              color: Colors.blue.shade600,
                            ),
                            const SizedBox(width: 8),
                            Expanded(
                              child: Text(
                                _deviceInfo[index],
                                style: const TextStyle(fontSize: 14),
                              ),
                            ),
                          ],
                        ),
                      );
                    },
                  ),
                ),
              ),
            ] else ...[
              const Expanded(
                child: Center(
                  child: Text(
                    "No device information available.\nPress 'Scan for KL520 Devices' to begin.",
                    textAlign: TextAlign.center,
                    style: TextStyle(
                      fontSize: 16,
                      color: Colors.grey,
                    ),
                  ),
                ),
              ),
            ],
          ],
        ),
      ),
    );
  }
}