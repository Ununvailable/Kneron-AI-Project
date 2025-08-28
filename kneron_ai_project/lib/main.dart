import 'dart:ffi';
import 'dart:convert';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:flutter/foundation.dart';
import 'package:ffi/ffi.dart'; // Import ffi for String conversions
import 'kl520_ffi.dart';

// Define a MethodChannel for USB initialization
const platform = MethodChannel('kl520_usb_plugin/usb');

Future<void> initializeUsbBridge() async {
  try {
    final result = await platform.invokeMethod('initializeUsb');
    debugPrint("UsbHostBridge initialized with result: $result");
  } on PlatformException catch (e) {
    debugPrint("Failed to initialize UsbHostBridge: ${e.message}");
  }
}

String arrayToDartString(Array<Uint8> arr, int arrLength) {
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

  // Log buffer captures both debugPrint and scan messages
  final List<String> _logBuffer = [];
  final int _maxLogLines = 200;

  void _appendLog(String message) {
    setState(() {
      if (_logBuffer.length >= _maxLogLines) {
        _logBuffer.removeAt(0);
      }
      _logBuffer.add(message);
    });
  }

  @override
  void initState() {
    super.initState();

    // Override debugPrint to also capture logs in _logBuffer
    debugPrint = (String? message, {int? wrapWidth}) {
      if (message != null) _appendLog(message);
      debugPrintSynchronously(message, wrapWidth: wrapWidth);
    };

    initializeUsbBridge();
  }

  Future<void> _scanDevices() async {
    setState(() {
      _isScanning = true;
      _scanResult = "Scanning for KL520 devices...";
      _appendLog(_scanResult);
    });

    try {
      final Pointer<KpDevicesListC> devicesListPtr =
          kpUsbScanDevices().cast<KpDevicesListC>();

      if (devicesListPtr != nullptr) {
        final KpDevicesListC devicesList = devicesListPtr.ref;
        final int numDevices = devicesList.numDev;

        if (numDevices > 0) {
          _scanResult = "Found **$numDevices** Kneron devices!";
          _appendLog(_scanResult);

          for (int i = 0; i < numDevices; i++) {
            final KpDeviceDescriptorC device = devicesList.device[i];
            final firmware = arrayToDartString(device.firmware, 30);
            final portPath = arrayToDartString(device.portPath, 20);

            _appendLog(
              "**Device ${i + 1}:**\n"
              "  Port ID: ${device.portId}\n"
              "  Vendor ID: 0x${device.vendorId.toRadixString(16)}\n"
              "  Product ID: 0x${device.productId.toRadixString(16)}\n"
              "  Link Speed: ${device.linkSpeed}\n"
              "  KN Number: 0x${device.knNumber.toRadixString(16)}\n"
              "  Connectable: ${device.isConnectable}\n"
              "  Port Path: $portPath\n"
              "  Firmware: $firmware",
            );
          }
        } else {
          _scanResult = "No Kneron devices found";
          _appendLog(_scanResult);
          _appendLog("Check USB connections and device status.");
        }
      } else {
        _scanResult = "Error: Failed to retrieve device list pointer.";
        _appendLog(_scanResult);
      }
    } catch (e) {
      _scanResult = "Error during scan: **$e**";
      _appendLog(_scanResult);
      _appendLog(e.toString());
    } finally {
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
              label:
                  Text(_isScanning ? "Scanning..." : "Scan for KL520 Devices"),
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
                child: Text(
                  "Scan Result: $_scanResult",
                  style: const TextStyle(fontSize: 16),
                ),
              ),
            ),
            const SizedBox(height: 16),
            Expanded(
              child: Card(
                child: ListView.builder(
                  padding: const EdgeInsets.all(16),
                  itemCount: _logBuffer.length,
                  itemBuilder: (context, index) {
                    return Padding(
                      padding: const EdgeInsets.symmetric(vertical: 4),
                      child: Row(
                        crossAxisAlignment: CrossAxisAlignment.start,
                        children: [
                          Icon(
                            Icons.bug_report,
                            size: 16,
                            color: Colors.orange.shade700,
                          ),
                          const SizedBox(width: 8),
                          Expanded(
                            child: Text(
                              _logBuffer[index],
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
          ],
        ),
      ),
    );
  }
}



