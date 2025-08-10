package kl520_usb_plugin

// Add all missing Android USB imports
import android.hardware.usb.UsbDevice
import android.hardware.usb.UsbInterface
import android.hardware.usb.UsbEndpoint
import android.hardware.usb.UsbDeviceConnection
import android.hardware.usb.UsbManager
import android.util.Log

// TODO: Add correct import for KpDeviceDescriptor from your Kneron SDK
// This might be from a .jar/.aar file you need to include
// import com.kneron.android.KpDeviceDescriptor

/**
 * NOTE: Some fields like 'linkSpeed', 'knNumber', and 'portPath' are difficult to directly map
 * from Android's `UsbDevice`/`UsbDeviceConnection` API without custom enumeration logic or
 * additional Kneron-specific control transfers. They serve as placeholders here.
 */
data class KpDeviceDescriptor(
    val portId: Int = 0, // Maps to UsbDevice.getDeviceId() [13, 16]
    val vendorId: Int = 0, // Maps to UsbDevice.getVendorId() [13, 16]
    val productId: Int = 0, // Maps to UsbDevice.getProductId() [13, 16]
    val linkSpeed: Int = 0, // Corresponds to `kp_usb_speed_t` [13, 17]
    val knNumber: Long = 0, // Kneron-specific KN number [13, 15]
    val isConnectable: Boolean = false, // Indicates if the device is connectable [15, 18]
    val portPath: String = "", // Low-level USB port path (difficult to get directly on Android) [15, 18]
    val firmware: String = "" // Firmware description (e.g., from UsbDevice.getVersion() or ProductName) [15, 16, 18]
)

/**
 * This data class encapsulates the Android USB objects relevant to a connected Kneron device.
 * It also includes the `KpDeviceDescriptor` for Kneron-specific information.
 * This object will be passed to the C-side to enable JNI calls using these specific Android USB entities.
 */
data class KpUsbDevice(
    val usbDevice: UsbDevice, // The Android UsbDevice object
    val usbConnection: UsbDeviceConnection, // The open Android UsbDeviceConnection
    val usbInterface: UsbInterface, // The claimed Android UsbInterface
    val endpointCmdIn: UsbEndpoint, // Kneron command IN endpoint [19, 20]
    val endpointCmdOut: UsbEndpoint, // Kneron command OUT endpoint [19, 20]
    val endpointLogIn: UsbEndpoint, // Kneron log IN endpoint (optional) [19, 20]
    val deviceDescriptor: KpDeviceDescriptor, // Kneron-specific device information
    val firmwareSerial: Int // From `kp_usb_device_t`, for KL520 workaround [19, 20]
)

/**
 * This class acts as the JNI bridge, providing methods that the C++ native layer will call.
 * It performs actual USB operations using Android's `UsbDeviceConnection`.
 * It is crucial that the `usbDeviceConnection` passed to this class is already open and its
 * corresponding `UsbInterface` has been claimed by the Android application.
 */
class UsbHostBridge(private val usbDeviceConnection: UsbDeviceConnection) {

    /**
     * Performs a bulk transfer out (Host to Device) [10].
     * @param endpoint The `UsbEndpoint` object for the OUT transfer [10, 11].
     * @param data The byte array containing data to send [10, 12].
     * @param offset Offset in the data buffer [10, 12].
     * @param length The number of bytes to send from the buffer [10, 12].
     * @param timeout Timeout in milliseconds [10, 12].
     * @return The number of bytes actually transferred, or negative for error.
     */
    fun bulkTransferOut(endpoint: UsbEndpoint, data: ByteArray, offset: Int, length: Int, timeout: Int): Int {
        Log.d("UsbHostBridge", "bulkTransferOut: EP=${String.format("0x%02X", endpoint.address)}, Len=$length, Timeout=$timeout")
        return usbDeviceConnection.bulkTransfer(endpoint, data, offset, length, timeout)
    }

    /**
     * Performs a bulk transfer in (Device to Host) [10].
     * @param endpoint The `UsbEndpoint` object for the IN transfer [10, 12].
     * @param buffer The byte array to receive data [10, 12].
     * @param offset Offset in the buffer to write data [10, 12].
     * @param length The maximum number of bytes to receive [10, 12].
     * @param timeout Timeout in milliseconds [10, 12].
     * @return The number of bytes actually transferred, or negative for error.
     */
    fun bulkTransferIn(endpoint: UsbEndpoint, buffer: ByteArray, offset: Int, length: Int, timeout: Int): Int {
        Log.d("UsbHostBridge", "bulkTransferIn: EP=${String.format("0x%02X", endpoint.address)}, Len=$length, Timeout=$timeout")
        return usbDeviceConnection.bulkTransfer(endpoint, buffer, offset, length, timeout)
    }

    /**
     * Performs a control transfer [9].
     * @param requestType The type of the request (e.g., `UsbConstants.USB_TYPE_VENDOR | UsbConstants.USB_DIR_OUT`) [9].
     * @param request The request code [9].
     * @param value The value field of the setup packet [9].
     * @param index The index field of the setup packet [9].
     * @param buffer The byte array for data stage (can be null for no data stage) [9].
     * @param offset Offset in the buffer [10].
     * @param length The length of data to transfer [9].
     * @param timeout Timeout in milliseconds [9].
     * @return The number of bytes actually transferred, or negative for error.
     */
    fun controlTransfer(requestType: Int, request: Int, value: Int, index: Int, buffer: ByteArray?, offset: Int, length: Int, timeout: Int): Int {
        Log.d("UsbHostBridge", "controlTransfer: Type=${String.format("0x%02X", requestType)}, Req=${String.format("0x%02X", request)}, Val=${String.format("0x%04X", value)}, Index=${String.format("0x%04X", index)}, Len=$length, Timeout=$timeout")
        return usbDeviceConnection.controlTransfer(requestType, request, value, index, buffer, offset, length, timeout)
    }

    /**
     * Native methods to be implemented in C/C++ via JNI.
     * These are called from the Kotlin side to initialize and clean up the native JNI bridge.
     */
    external fun registerNative(): Int
    external fun cleanupNative()

    companion object {
        init {
            // Load the native library when this class is initialized
            System.loadLibrary("kneron_usb_jni_bridge") // Make sure this matches your library name
        }
    }
}