import 'dart:ffi';
import 'package:flutter/material.dart';
import 'package:ffi/ffi.dart';
import 'kl520_ffi.dart';

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
  List<String> _deviceInfo = [];

  Future<void> _scanDevices() async {
    setState(() {
      _isScanning = true;
      _scanResult = "Scanning for KL520 devices...";
      _deviceInfo.clear();
    });

    try {
      // Call the USB scan function
      final devicesList = kpUsbScanDevices();
      
      if (devicesList != nullptr) {
        setState(() {
          _scanResult = "Devices found! Device list pointer: ${devicesList.address.toRadixString(16)}";
          _deviceInfo.add("Device scan successful");
          _deviceInfo.add("Found KL520 USB devices");
          _deviceInfo.add("Device list address: 0x${devicesList.address.toRadixString(16)}");
        });
      } else {
        setState(() {
          _scanResult = "No KL520 devices found";
          _deviceInfo.add("No devices detected");
          _deviceInfo.add("Check USB connections");
        });
      }
    } catch (e) {
      setState(() {
        _scanResult = "Error during scan: $e";
        _deviceInfo.add("Scan failed with error:");
        _deviceInfo.add(e.toString());
      });
    } finally {
      setState(() {
        _isScanning = false;
      });
    }
  }

  Future<void> _testHighLevelScan() async {
    setState(() {
      _isScanning = true;
      _scanResult = "Testing high-level scan function...";
      _deviceInfo.clear();
    });

    try {
      if (scanDevices != null) {
        // Create dummy argc/argv for the function call
        final argc = 1;
        final argv = calloc<Pointer<Utf8>>();
        argv.value = "kl520_app".toNativeUtf8();

        final result = scanDevices!(argc, argv);
        
        calloc.free(argv.value);
        calloc.free(argv);

        setState(() {
          _scanResult = "High-level scan completed with result: $result";
          _deviceInfo.add("Scan function returned: $result");
          _deviceInfo.add(result == 0 ? "Success" : "Error code: $result");
        });
      } else {
        setState(() {
          _scanResult = "High-level scan function not available";
          _deviceInfo.add("kp_scan_devices symbol not found in library");
        });
      }
    } catch (e) {
      setState(() {
        _scanResult = "Error in high-level scan: $e";
        _deviceInfo.add("High-level scan failed:");
        _deviceInfo.add(e.toString());
      });
    } finally {
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
            // Main scan button
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
            
            const SizedBox(height: 16),
            
            // High-level scan button
            ElevatedButton.icon(
              onPressed: _isScanning ? null : _testHighLevelScan,
              icon: const Icon(Icons.settings),
              label: const Text("Test High-Level Scan"),
              style: ElevatedButton.styleFrom(
                backgroundColor: Colors.green.shade600,
                foregroundColor: Colors.white,
                padding: const EdgeInsets.symmetric(vertical: 16),
              ),
            ),
            
            const SizedBox(height: 24),
            
            // Results section
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
                            : _scanResult.contains("found") 
                                ? Colors.green.shade700
                                : Colors.black87,
                      ),
                    ),
                  ],
                ),
              ),
            ),
            
            const SizedBox(height: 16),
            
            // Device info section
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