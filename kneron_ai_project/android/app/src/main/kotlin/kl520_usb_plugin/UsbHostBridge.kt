package kl520_usb_plugin

import android.content.Context
import android.hardware.usb.UsbConstants
import android.hardware.usb.UsbDevice
import android.hardware.usb.UsbDeviceConnection
import android.hardware.usb.UsbEndpoint
import android.hardware.usb.UsbInterface
import android.hardware.usb.UsbManager
import android.util.Log
import android.app.PendingIntent
import android.content.Intent
import android.content.IntentFilter

// --- Data Classes (remain as is, well-defined) ---

/**
 * Represents Kneron-specific device information. Some fields are placeholders due to direct mapping
 * difficulty from Android APIs without custom enumeration logic or additional control transfers.
 */
data class KpDeviceDescriptor(
    val portId: Int = 0, // Maps to UsbDevice.getDeviceId() [3]
    val vendorId: Int = 0, // Maps to UsbDevice.getVendorId() [3]
    val productId: Int = 0, // Maps to UsbDevice.getProductId() [3]
    val linkSpeed: Int = 0, // Corresponds to `kp_usb_speed_t` [3]
    val knNumber: Long = 0, // Kneron-specific KN number [3]
    val isConnectable: Boolean = false, // Indicates if the device is connectable [4]
    val portPath: String = "", // Low-level USB port path (difficult to get directly on Android) [4]
    val firmware: String = "" // Firmware description (e.g., from UsbDevice.getVersion() or ProductName) [4]
) {
    // Helper to convert to a Map for MethodChannel results in Dart
    fun toMap(): Map<String, Any> {
        return mapOf(
            "portId" to portId,
            "vendorId" to vendorId,
            "productId" to productId,
            "linkSpeed" to linkSpeed,
            "knNumber" to knNumber,
            "isConnectable" to isConnectable,
            "portPath" to portPath,
            "firmware" to firmware
        )
    }
}

/**
 * This data class encapsulates the Android USB objects relevant to a connected Kneron device.
 * It also includes the `KpDeviceDescriptor` for Kneron-specific information.
 * This object will be passed to the C-side to enable JNI calls using these specific Android USB entities. [5]
 */
data class KpUsbDevice(
    val usbDevice: UsbDevice, // The Android UsbDevice object [5]
    val usbConnection: UsbDeviceConnection, // The open Android UsbDeviceConnection [5]
    val usbInterface: UsbInterface, // The claimed Android UsbInterface [5]
    val endpointCmdIn: UsbEndpoint, // Kneron command IN endpoint [5, 18, 19]
    val endpointCmdOut: UsbEndpoint, // Kneron command OUT endpoint [5, 18, 19]
    val endpointLogIn: UsbEndpoint?, // Kneron log IN endpoint (optional) [5, 18, 19]
    val deviceDescriptor: KpDeviceDescriptor, // Kneron-specific device information [5]
    val firmwareSerial: Int // From `kp_usb_device_t`, for KL520 workaround [6, 18, 19]
)

// --- UsbHostBridge Class (Refactored) ---

/**
 * This class acts as the JNI bridge and the central manager for Android USB host operations.
 * It provides methods for scanning, connecting, and performing USB transfers.
 * It holds the Android Context to access UsbManager, but does NOT hold a single UsbDeviceConnection
 * as a member, allowing it to manage multiple connected devices via the JNI layer.
 */
class UsbHostBridge(private val context: Context) {

    // Kneron USB Vendor ID [20, 21]
    private val KNERON_VENDOR_ID = 0x3231

    // Lazily initialized UsbManager using the provided Context
    private val usbManager: UsbManager by lazy {
        context.getSystemService(Context.USB_SERVICE) as UsbManager
    }

    // A map to store active KpUsbDevice connections, keyed by deviceId or portId for easy lookup.
    // This allows the UsbHostBridge to manage multiple devices.
    private val activeConnections: MutableMap<Int, KpUsbDevice> = mutableMapOf()

    // --- Companion Object (for static library loading) ---
    companion object {
        init {
            // Load the native library when this class is initialized [2]
            System.loadLibrary("kp_usb_jni") // Make sure this matches your library name
        }
        // --- Native Methods (Instance methods, called by JNI) ---
        
        /**
         * Native method to be implemented in C/C++ via JNI. [2]
         * This is called from the Kotlin side to initialize the native JNI bridge.
         * The `jobject thiz` in the C implementation (e.g., `usb_jni_initialize`) will receive this `UsbHostBridge` instance.
         * @return 0 on success, negative on error.
         */
        @JvmStatic
        external fun registerNative(): Int

        /**
         * Native method to be implemented in C/C++ via JNI. [2]
         * This is called from the Kotlin side to clean up native JNI bridge resources.
         * @return 0 on success, negative on error.
         */
        @JvmStatic
        external fun cleanupNative(): Int // Added Int return type for consistency with registerNative

    }

    // --- USB Host Operations (Kotlin-side methods, potentially called from Dart via MethodChannel) ---

    /**
     * Scans for connected Kneron USB devices and returns a list of their descriptors.
     * This method is expected to be called by the JNI `usb_jni_scan_devices()` [22].
     * @return An array of [KpDeviceDescriptor] objects for detected Kneron devices.
     */
    fun scanKneronDevices(): Array<KpDeviceDescriptor> {
        val deviceList = usbManager.deviceList
        val kneronDevices = mutableListOf<KpDeviceDescriptor>()

        for ((_, usbDevice) in deviceList) {
            if (usbDevice.vendorId == KNERON_VENDOR_ID) {
                // Populate KpDeviceDescriptor.
                // Note: 'linkSpeed', 'knNumber', 'portPath' are difficult to get directly
                // from UsbDevice without custom control transfers or deeper OS interaction.
                // We'll use best available information or placeholders.
                val descriptor = KpDeviceDescriptor(
                    portId = usbDevice.deviceId, // Using Android's deviceId as portId [3]
                    vendorId = usbDevice.vendorId,
                    productId = usbDevice.productId,
                    firmware = usbDevice.productName ?: "N/A", // Use product name for firmware [4]
                    isConnectable = usbManager.hasPermission(usbDevice), // Check for permission [4]
                    // Link speed, KN number, port path are harder to get from UsbDevice directly.
                    // The C-side `kp_usb.c` [23, 24] has logic for calculating port_id and port_path.
                    // For now, we rely on the C layer for accurate mapping if needed or use placeholders.
                    linkSpeed = 0, // Placeholder [3]
                    knNumber = 0L, // Placeholder [3]
                    portPath = usbDevice.deviceName ?: "" // Use device name as a simple path [4]
                )
                kneronDevices.add(descriptor)
                Log.d("UsbHostBridge", "Found Kneron device: ${descriptor.firmware} (VID: ${descriptor.vendorId}, PID: ${descriptor.productId}, PortID: ${descriptor.portId})")
            }
        }
        return kneronDevices.toTypedArray()
    }

    /**
     * Attempts to open a specific Kneron device based on its descriptor.
     * This involves getting permission, opening a connection, and claiming the interface.
     * @param deviceDescriptor The descriptor of the device to open.
     * @return A [KpUsbDevice] object if successful, null otherwise.
     */
    fun openDevice(deviceDescriptor: KpDeviceDescriptor): KpUsbDevice? {
        val usbDevice = usbManager.deviceList.values.firstOrNull {
            it.vendorId == deviceDescriptor.vendorId &&
            it.productId == deviceDescriptor.productId &&
            it.deviceId == deviceDescriptor.portId // Match by deviceId (used as portId)
        } ?: run {
            Log.e("UsbHostBridge", "UsbDevice not found for descriptor: $deviceDescriptor")
            return null
        }

        // Check/request permission (actual permission flow often involves UI and a broadcast receiver)
        if (!usbManager.hasPermission(usbDevice)) {
            Log.w("UsbHostBridge", "Permission not granted for device: ${usbDevice.deviceName}. Requesting permission.")
            // This is a simplified permission request. A real app would use a BroadcastReceiver
            // and a more robust mechanism to wait for the permission result.
            val permissionIntent = PendingIntent.getBroadcast(
                context, 0, Intent("com.android.example.USB_PERMISSION"),
                PendingIntent.FLAG_IMMUTABLE or PendingIntent.FLAG_UPDATE_CURRENT
            )
            usbManager.requestPermission(usbDevice, permissionIntent)
            Log.e("UsbHostBridge", "USB permission needs to be granted by the user. " +
                    "This method cannot block for the result. You'll need to re-call after permission is granted.")
            return null // Return null, as permission is async.
        }

        val connection = usbManager.openDevice(usbDevice)
        if (connection == null) {
            Log.e("UsbHostBridge", "Failed to open UsbDeviceConnection for device: ${usbDevice.deviceName}")
            return null
        }

        // Find and claim the first interface, and identify endpoints.
        // Assuming the Kneron device uses a single interface and specific endpoint types.
        var usbInterface: UsbInterface? = null
        var endpointCmdIn: UsbEndpoint? = null
        var endpointCmdOut: UsbEndpoint? = null
        var endpointLogIn: UsbEndpoint? = null // Optional log endpoint [5]

        for (i in 0 until usbDevice.interfaceCount) {
            val iface = usbDevice.getInterface(i)
            // Assuming the first interface is the primary for commands.
            // In a real scenario, you'd check interface class/subclass.
            if (iface.endpointCount > 0) {
                usbInterface = iface
                break
            }
        }

        if (usbInterface == null) {
            Log.e("UsbHostBridge", "No suitable UsbInterface found for device: ${usbDevice.deviceName}")
            connection.close()
            return null
        }

        if (!connection.claimInterface(usbInterface, true)) { // Force claim
            Log.e("UsbHostBridge", "Failed to claim UsbInterface for device: ${usbDevice.deviceName}")
            connection.close()
            return null
        }

        // Identify endpoints based on type and direction
        for (i in 0 until usbInterface.endpointCount) {
            val endpoint = usbInterface.getEndpoint(i)
            if (endpoint.type == UsbConstants.USB_ENDPOINT_XFER_BULK) {
                if (endpoint.direction == UsbConstants.USB_DIR_IN) {
                    endpointCmdIn = endpoint // Kneron command IN endpoint [5]
                } else {
                    endpointCmdOut = endpoint // Kneron command OUT endpoint [5]
                }
            }
            // Add logic for Interrupt endpoints for logs if needed (endpointLogIn) [5]
            if (endpoint.type == UsbConstants.USB_ENDPOINT_XFER_INT && endpoint.direction == UsbConstants.USB_DIR_IN) {
                 endpointLogIn = endpoint // Kneron log IN endpoint [5]
            }
        }

        if (endpointCmdIn == null || endpointCmdOut == null) {
            Log.e("UsbHostBridge", "Required bulk endpoints not found for device: ${usbDevice.deviceName}")
            connection.releaseInterface(usbInterface)
            connection.close()
            return null
        }

        val kpUsbDevice = KpUsbDevice(
            usbDevice = usbDevice,
            usbConnection = connection,
            usbInterface = usbInterface,
            endpointCmdIn = endpointCmdIn,
            endpointCmdOut = endpointCmdOut,
            endpointLogIn = endpointLogIn,
            deviceDescriptor = deviceDescriptor,
            firmwareSerial = usbDevice.version.toIntOrNull() ?: 0 // Using bcdDevice / version [6, 25]
        )
        activeConnections[kpUsbDevice.deviceDescriptor.portId] = kpUsbDevice
        Log.d("UsbHostBridge", "Successfully opened device: ${kpUsbDevice.deviceDescriptor.firmware}")
        return kpUsbDevice
    }

    /**
     * Closes the connection for a specific Kneron device.
     * @param kpUsbDevice The [KpUsbDevice] object to close.
     */
    fun closeDevice(kpUsbDevice: KpUsbDevice) {
        kpUsbDevice.usbConnection.releaseInterface(kpUsbDevice.usbInterface)
        kpUsbDevice.usbConnection.close()
        activeConnections.remove(kpUsbDevice.deviceDescriptor.portId)
        Log.d("UsbHostBridge", "Closed connection for device: ${kpUsbDevice.deviceDescriptor.firmware}")
    }

    // --- USB Transfer Methods (Modified to accept UsbDeviceConnection and UsbEndpoint) ---

    /**
     * Performs a bulk transfer out (Host to Device). [7]
     * **NOTE:** This method's signature *differs* from the one looked up by JNI in `kp_usb_jni.c`
     * (which is `(Landroid/hardware/usb/UsbEndpoint;[BIII)I` [17]).
     * This refactoring assumes that `kp_usb_jni.c` would be modified to pass the `UsbDeviceConnection`
     * explicitly. If `kp_usb_jni.c` cannot change, a different approach would be needed (e.g., mapping
     * endpoint addresses to connections on the Kotlin side, which is less robust).
     *
     * @param connection The specific `UsbDeviceConnection` to use for the transfer.
     * @param endpoint The `UsbEndpoint` object for the OUT transfer. [9, 10]
     * @param data The byte array containing data to send. [9, 11]
     * @param offset Offset in the data buffer. [9, 11]
     * @param length The number of bytes to send from the buffer. [9, 11]
     * @param timeout Timeout in milliseconds. [9, 11]
     * @return The number of bytes actually transferred, or negative for error.
     */
    fun bulkTransferOut(connection: UsbDeviceConnection, endpoint: UsbEndpoint, data: ByteArray, offset: Int, length: Int, timeout: Int): Int {
        Log.d("UsbHostBridge", "bulkTransferOut: EP=${String.format("0x%02X", endpoint.address)}, Len=$length, Timeout=$timeout")
        return connection.bulkTransfer(endpoint, data, offset, length, timeout)
    }

    /**
     * Performs a bulk transfer in (Device to Host). [9]
     * **NOTE:** Similar to `bulkTransferOut`, this method's signature *differs* from the
     * one looked up by JNI in `kp_usb_jni.c` [26].
     *
     * @param connection The specific `UsbDeviceConnection` to use for the transfer.
     * @param endpoint The `UsbEndpoint` object for the IN transfer. [9, 11]
     * @param buffer The byte array to receive data. [9, 11]
     * @param offset Offset in the buffer to write data. [9, 11]
     * @param length The maximum number of bytes to receive. [9, 11]
     * @param timeout Timeout in milliseconds. [9, 11]
     * @return The number of bytes actually transferred, or negative for error.
     */
    fun bulkTransferIn(connection: UsbDeviceConnection, endpoint: UsbEndpoint, buffer: ByteArray, offset: Int, length: Int, timeout: Int): Int {
        Log.d("UsbHostBridge", "bulkTransferIn: EP=${String.format("0x%02X", endpoint.address)}, Len=$length, Timeout=$timeout")
        return connection.bulkTransfer(endpoint, buffer, offset, length, timeout)
    }

    /**
     * Performs a control transfer. [8]
     * **NOTE:** Similar to bulk transfers, this method's signature *differs* from the
     * one looked up by JNI in `kp_usb_jni.c` [27].
     *
     * @param connection The specific `UsbDeviceConnection` to use for the transfer.
     * @param requestType The type of the request (e.g., `UsbConstants.USB_TYPE_VENDOR | UsbConstants.USB_DIR_OUT`). [8]
     * @param request The request code. [8]
     * @param value The value field of the setup packet. [8]
     * @param index The index field of the setup packet. [8]
     * @param buffer The byte array for data stage (can be null for no data stage). [8]
     * @param offset Offset in the buffer. [9]
     * @param length The length of data to transfer. [8]
     * @param timeout Timeout in milliseconds. [8]
     * @return The number of bytes actually transferred, or negative for error.
     */
    fun controlTransfer(connection: UsbDeviceConnection, requestType: Int, request: Int, value: Int, index: Int, buffer: ByteArray?, offset: Int, length: Int, timeout: Int): Int {
        Log.d("UsbHostBridge", "controlTransfer: Type=${String.format("0x%02X", requestType)}, Req=${String.format("0x%02X", request)}, Val=${String.format("0x%04X", value)}, Index=${String.format("0x%04X", index)}, Len=$length, Timeout=$timeout")
        return connection.controlTransfer(requestType, request, value, index, buffer, offset, length, timeout)
    }

    // You might also need a `readFirmwareLog` method, which currently has a stub in `kp_usb.c` [28].
    // If it relies on an Interrupt IN endpoint, its signature would also need to be adjusted.
    // E.g., fun readFirmwareLog(connection: UsbDeviceConnection, endpoint: UsbEndpoint, buffer: ByteArray, offset: Int, length: Int, timeout: Int): Int
}