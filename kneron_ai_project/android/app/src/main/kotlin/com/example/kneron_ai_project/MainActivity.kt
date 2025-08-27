package com.example.kneron_ai_project

import io.flutter.embedding.android.FlutterActivity
import io.flutter.embedding.engine.FlutterEngine
import io.flutter.plugin.common.MethodChannel
import android.util.Log // Import Log for debugging output

// Assuming UsbHostBridge.kt is in the 'kl520_usb_plugin' package
// This import allows access to the UsbHostBridge class and its companion object methods.
import kl520_usb_plugin.UsbHostBridge

class MainActivity : FlutterActivity() {
    // Define the MethodChannel name, which must match the one used in main.dart [2]
    private val CHANNEL = "kl520_usb_plugin/usb"

    override fun configureFlutterEngine(flutterEngine: FlutterEngine) {
        // Call the super method to ensure Flutter engine is configured correctly
        super.configureFlutterEngine(flutterEngine)

        // Set up the MethodChannel to listen for calls from Dart
        MethodChannel(flutterEngine.dartExecutor.binaryMessenger, CHANNEL).setMethodCallHandler {
            call, result ->
            // Check if the incoming method call is "initializeUsb"
            if (call.method == "initializeUsb") {
                // The UsbHostBridge.kt companion object's init block loads the native library "kp_usb_jni" [4].
                // This call triggers the JNI initialization on the C side [5, 6].
                // The C function usb_jni_initialize expects a jobject, which in this static context, would be the Class object of UsbHostBridge.
                val initResult = UsbHostBridge.registerNative()

                if (initResult == 0) { // Assuming 0 indicates success as per C's usb_jni_initialize [7]
                    Log.d("MainActivity", "UsbHostBridge.registerNative() called successfully. JNI bridge initialized.")
                    result.success(true) // Report success back to Dart
                } else {
                    Log.e("MainActivity", "Failed to call UsbHostBridge.registerNative(), result code: $initResult")
                    // Report an error back to Dart with a detailed message
                    result.error("INIT_FAILED", "Failed to initialize native USB bridge: Error code $initResult", null)
                }
            } else {
                // If the method is not recognized, report it as not implemented
                result.notImplemented()
            }
        }
    }

    override fun onDestroy() {
        super.onDestroy()
        // It's good practice to clean up native resources when the activity is destroyed.
        // The cleanupNative method in the UsbHostBridge companion object [4] calls
        // usb_jni_finalize on the C side to free global references and memory [7, 8].
        val cleanupResult = UsbHostBridge.cleanupNative()
        Log.d("MainActivity", "UsbHostBridge.cleanupNative() called on MainActivity onDestroy, result: $cleanupResult")
    }
}