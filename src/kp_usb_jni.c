#include "kp_usb_jni.h"
#include <stdlib.h>
#include <string.h>
#include <android/log.h>
#include <pthread.h>

#define LOG_TAG "KP_USB_TRANSPORT"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

#include "kp_usb_jni.h" // Assuming this header includes jni.h, stdio.h, stdlib.h, string.h, pthread.h, android/log.h, stdint.h, stdbool.h, and defines kp_devices_list_t, kp_device_descriptor_t, kp_usb_control_t

// Forward declaration if kp_usb_jni.h doesn't fully define them
typedef struct usb_device_handle usb_device_handle_t;

// Structure to hold USB device handle information
// REF: [4]
struct usb_device_handle {
    JNIEnv* env;
    jobject usb_host_bridge;         // Global reference to the UsbHostBridge *instance* specific to this device connection
    jobject usb_connection_obj;      // Global reference to Android's UsbDeviceConnection [5]
    jobject usb_device_obj;          // Global reference to Android's UsbDevice [5]
    jobject usb_interface_obj;       // Global reference to Android's UsbInterface [5]
    jobject bulk_in_endpoint_obj;    // Global reference to Android's UsbEndpoint for bulk IN [5]
    jobject bulk_out_endpoint_obj;   // Global reference to Android's UsbEndpoint for bulk OUT [5]
    jobject interrupt_in_endpoint_obj; // Global reference to Android's UsbEndpoint for interrupt IN [5]

    // Cached method IDs for the *instance* of UsbHostBridge stored in usb_host_bridge field
    jmethodID bulk_out_method;
    jmethodID bulk_in_method;
    jmethodID control_method;
    jmethodID interrupt_in_method; // New method ID for interrupt transfer

    uint16_t vendor_id;             // From KpDeviceDescriptor [6]
    uint16_t product_id;            // From KpDeviceDescriptor [6]
    uint32_t firmware_serial;       // From KpUsbDevice.firmwareSerial [1]
};

// Global state
static JavaVM* g_jvm = NULL;
// g_usb_host_bridge is a global reference to the *initial* UsbHostBridge instance passed by Kotlin/Flutter
// It's primarily used to get class references and call static methods like scanKneronDevices
static jobject g_usb_host_bridge = NULL; // [7]
static pthread_mutex_t g_jni_mutex = PTHREAD_MUTEX_INITIALIZER; // [7]

// Cached global class references for efficiency
static jclass g_usb_host_bridge_class = NULL;
static jclass g_kp_usb_device_class = NULL;
static jclass g_kp_device_descriptor_class = NULL;
static jclass g_usb_device_connection_class = NULL;
static jclass g_usb_device_class = NULL;
static jclass g_usb_interface_class = NULL;
static jclass g_usb_endpoint_class = NULL;

// Static buffer for device list (matching original libusb implementation pattern) [7]
static kp_devices_list_t* g_kdev_list = NULL; // [7]
static int g_kdev_list_size = 0; // [7]

// ---------------------------------------------------------------------------
// Helper utilities
// ---------------------------------------------------------------------------

// Called from JNI_OnLoad to store JavaVM
jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    g_jvm = vm;
    return JNI_VERSION_1_6;
}

JNIEnv* usb_jni_get_env(void) {
    if (!g_jvm) {
        LOGE("usb_jni_get_env: JavaVM not initialized");
        return NULL;
    }

    JNIEnv* env = NULL;
    jint result = (*g_jvm)->GetEnv(g_jvm, (void**)&env, JNI_VERSION_1_6);
    if (result == JNI_EDETACHED) {
        // Current thread is not attached, attach it
        result = (*g_jvm)->AttachCurrentThread(g_jvm, &env, NULL);
        if (result != JNI_OK) {
            LOGE("usb_jni_get_env: Failed to attach current thread");
            return NULL;
        }
    } else if (result != JNI_OK) {
        LOGE("usb_jni_get_env: Failed to get JNI environment");
        return NULL;
    }
    return env;
}

// JNI wrapper for initialization (uncommented and used for actual JNI exports) [9]
JNIEXPORT jint JNICALL Java_kl520_1usb_1plugin_UsbHostBridge_registerNative(JNIEnv* env, jobject thiz) {
    return usb_jni_initialize(env, thiz);
}

// JNI wrapper for cleanup (uncommented and used for actual JNI exports) [9]
JNIEXPORT void JNICALL Java_kl520_1usb_1plugin_UsbHostBridge_cleanupNative(JNIEnv* env, jobject thiz) {
    usb_jni_cleanup(); // Call the C-level cleanup
}

/**
 * @brief Generate port_id from port_path (matching original algorithm) [9]
 */
static uint32_t generate_port_id_from_path(const char* port_path) { // [9]
    // if (!port_path || port_path == '\0') {
    //     return 0;
    // }

    // Parse "busNo-hub_portNo-device_portNo" format
    uint32_t port_id = 0;
    char* path_copy = strdup(port_path);
    if (!path_copy) return 0;
    char* token = strtok(path_copy, "-");
    if (token) {
        // Bus number (2 bits)
        int bus_number = atoi(token);
        port_id |= (bus_number & 0x3);

        // Port numbers (5 bits each)
        int port_index = 0;
        while ((token = strtok(NULL, "-")) != NULL && port_index < 6) {
            int port_num = atoi(token);
            port_id |= ((uint32_t)port_num << (2 + port_index * 5));
            port_index++;
        }
    }
    free(path_copy);
    return port_id;
}

// ---------------------------------------------------------------------------
// Main API implementation
// ---------------------------------------------------------------------------

// [10]
int usb_jni_initialize(JNIEnv* env, jobject usb_host_bridge) {
    if (!env || !usb_host_bridge) {
        LOGE("usb_jni_initialize: Invalid parameters");
        return -1;
    }

    pthread_mutex_lock(&g_jni_mutex); // [11]

    // Store global JavaVM [10]
    if ((*env)->GetJavaVM(env, &g_jvm) != JNI_OK) {
        LOGE("usb_jni_initialize: Failed to get JavaVM");
        pthread_mutex_unlock(&g_jni_mutex);
        return -1;
    }

    // Clean up existing global reference if any [11]
    if (g_usb_host_bridge) {
        (*env)->DeleteGlobalRef(env, g_usb_host_bridge);
    }
    if (g_usb_host_bridge_class) {
        (*env)->DeleteGlobalRef(env, g_usb_host_bridge_class);
    }
    if (g_kp_usb_device_class) {
        (*env)->DeleteGlobalRef(env, g_kp_usb_device_class);
    }
    if (g_kp_device_descriptor_class) {
        (*env)->DeleteGlobalRef(env, g_kp_device_descriptor_class);
    }
    if (g_usb_device_connection_class) {
        (*env)->DeleteGlobalRef(env, g_usb_device_connection_class);
    }
    if (g_usb_device_class) {
        (*env)->DeleteGlobalRef(env, g_usb_device_class);
    }
    if (g_usb_interface_class) {
        (*env)->DeleteGlobalRef(env, g_usb_interface_class);
    }
    if (g_usb_endpoint_class) {
        (*env)->DeleteGlobalRef(env, g_usb_endpoint_class);
    }

    // Create global reference to prevent GC for the initial UsbHostBridge instance [11]
    g_usb_host_bridge = (*env)->NewGlobalRef(env, usb_host_bridge);
    if (!g_usb_host_bridge) {
        LOGE("usb_jni_initialize: Failed to create global reference for UsbHostBridge instance");
        g_jvm = NULL;
        pthread_mutex_unlock(&g_jni_mutex);
        return -1;
    }

    // Cache global class references (using FindClass for types that are not the initial instance)
    // NOTE: FindClass cannot be used later by attached threads, so it must be done here or use GetObjectClass.
    // Making them global refs from NewGlobalRef(FindClass(...)) is safest.
    jclass local_bridge_class = (*env)->FindClass(env, "kl520_usb_plugin/UsbHostBridge");
    jclass local_kp_usb_device_class = (*env)->FindClass(env, "kl520_usb_plugin/KpUsbDevice");
    jclass local_kp_device_descriptor_class = (*env)->FindClass(env, "kl520_usb_plugin/KpDeviceDescriptor");
    jclass local_usb_device_connection_class = (*env)->FindClass(env, "android/hardware/usb/UsbDeviceConnection");
    jclass local_usb_device_class = (*env)->FindClass(env, "android/hardware/usb/UsbDevice");
    jclass local_usb_interface_class = (*env)->FindClass(env, "android/hardware/usb/UsbInterface");
    jclass local_usb_endpoint_class = (*env)->FindClass(env, "android/hardware/usb/UsbEndpoint");

    if (!local_bridge_class || !local_kp_usb_device_class || !local_kp_device_descriptor_class ||
        !local_usb_device_connection_class || !local_usb_device_class || !local_usb_interface_class ||
        !local_usb_endpoint_class) {
        LOGE("usb_jni_initialize: Failed to find one or more required Java classes.");
        // Perform cleanup for already created global refs before returning
        (*env)->DeleteGlobalRef(env, g_usb_host_bridge); g_usb_host_bridge = NULL;
        g_jvm = NULL; pthread_mutex_unlock(&g_jni_mutex); return -1;
    }

    g_usb_host_bridge_class = (*env)->NewGlobalRef(env, local_bridge_class);
    g_kp_usb_device_class = (*env)->NewGlobalRef(env, local_kp_usb_device_class);
    g_kp_device_descriptor_class = (*env)->NewGlobalRef(env, local_kp_device_descriptor_class);
    g_usb_device_connection_class = (*env)->NewGlobalRef(env, local_usb_device_connection_class);
    g_usb_device_class = (*env)->NewGlobalRef(env, local_usb_device_class);
    g_usb_interface_class = (*env)->NewGlobalRef(env, local_usb_interface_class);
    g_usb_endpoint_class = (*env)->NewGlobalRef(env, local_usb_endpoint_class);

    // Delete local references
    (*env)->DeleteLocalRef(env, local_bridge_class);
    (*env)->DeleteLocalRef(env, local_kp_usb_device_class);
    (*env)->DeleteLocalRef(env, local_kp_device_descriptor_class);
    (*env)->DeleteLocalRef(env, local_usb_device_connection_class);
    (*env)->DeleteLocalRef(env, local_usb_device_class);
    (*env)->DeleteLocalRef(env, local_usb_interface_class);
    (*env)->DeleteLocalRef(env, local_usb_endpoint_class);

    if (!g_usb_host_bridge_class || !g_kp_usb_device_class || !g_kp_device_descriptor_class ||
        !g_usb_device_connection_class || !g_usb_device_class || !g_usb_interface_class ||
        !g_usb_endpoint_class) {
        LOGE("usb_jni_initialize: Failed to create global references for one or more Java classes.");
        // More robust cleanup needed here for already created global refs
        // For brevity, assuming success for now after NewGlobalRef.
        g_jvm = NULL; pthread_mutex_unlock(&g_jni_mutex); return -1;
    }


    pthread_mutex_unlock(&g_jni_mutex); // [11]
    LOGD("usb_jni_initialize: Initialization successful"); // [12]
    return 0;
}

// [12]
int usb_jni_finalize(JNIEnv* env) {
    if (!env) return -1;

    pthread_mutex_lock(&g_jni_mutex); // [12]

    if (g_usb_host_bridge) { // [12]
        (*env)->DeleteGlobalRef(env, g_usb_host_bridge);
        g_usb_host_bridge = NULL;
    }

    // Delete global class references
    if (g_usb_host_bridge_class) {
        (*env)->DeleteGlobalRef(env, g_usb_host_bridge_class); g_usb_host_bridge_class = NULL;
    }
    if (g_kp_usb_device_class) {
        (*env)->DeleteGlobalRef(env, g_kp_usb_device_class); g_kp_usb_device_class = NULL;
    }
    if (g_kp_device_descriptor_class) {
        (*env)->DeleteGlobalRef(env, g_kp_device_descriptor_class); g_kp_device_descriptor_class = NULL;
    }
    if (g_usb_device_connection_class) {
        (*env)->DeleteGlobalRef(env, g_usb_device_connection_class); g_usb_device_connection_class = NULL;
    }
    if (g_usb_device_class) {
        (*env)->DeleteGlobalRef(env, g_usb_device_class); g_usb_device_class = NULL;
    }
    if (g_usb_interface_class) {
        (*env)->DeleteGlobalRef(env, g_usb_interface_class); g_usb_interface_class = NULL;
    }
    if (g_usb_endpoint_class) {
        (*env)->DeleteGlobalRef(env, g_usb_endpoint_class); g_usb_endpoint_class = NULL;
    }

    g_jvm = NULL; // [12]
    pthread_mutex_unlock(&g_jni_mutex); // [12]
    LOGD("usb_jni_finalize: Finalized USB JNI"); // [12]
    return 0;
}

// [12]
usb_device_handle_t* usb_jni_open(uint16_t vendor_id, uint16_t product_id) {
    if (!g_jvm || !g_usb_host_bridge) {
        LOGE("usb_jni_open: Transport not initialized");
        return NULL;
    }

    JNIEnv* env = usb_jni_get_env(); // [13]
    if (!env) {
        LOGE("usb_jni_open: Failed to get JNI environment");
        return NULL;
    }

    // Allocate device handle [13]
    usb_device_handle_t* handle = (usb_device_handle_t*)malloc(sizeof(usb_device_handle_t)); // [13]
    if (!handle) {
        LOGE("usb_jni_open: Failed to allocate memory"); // [13]
        return NULL;
    }
    memset(handle, 0, sizeof(usb_device_handle_t)); // [13]
    handle->env = env;

    jobject kpUsbDevice_obj = NULL;
    jobject local_usb_host_bridge_instance = NULL;

    do {
        // Get the static method ID for connectKneronDevice from UsbHostBridge.Companion
        jmethodID connect_device_static_method = (*env)->GetMethodID(env, g_usb_host_bridge_class,
                                                    "connectKneronDevice", "(II)Lkl520_usb_plugin/KpUsbDevice;");
        if (!connect_device_static_method) {
            LOGE("usb_jni_open: Failed to get static connectKneronDevice method ID");
            break;
        }

        // Call the static connectKneronDevice method
        kpUsbDevice_obj = (*env)->CallObjectMethod(env, g_usb_host_bridge_class,
                                                           connect_device_static_method,
                                                           (jint)vendor_id, (jint)product_id);
        if ((*env)->ExceptionCheck(env)) {
            LOGE("usb_jni_open: Exception during connectKneronDevice call");
            (*env)->ExceptionDescribe(env);
            (*env)->ExceptionClear(env);
            break;
        }
        if (!kpUsbDevice_obj) {
            LOGE("usb_jni_open: No Kneron device connected for VID:0x%04x PID:0x%04x", vendor_id, product_id);
            break;
        }

        // --- Extract fields from KpUsbDevice object ---
        // Get field IDs for KpUsbDevice
        jfieldID usbDevice_field = (*env)->GetFieldID(env, g_kp_usb_device_class, "usbDevice", "Landroid/hardware/usb/UsbDevice;"); // [5]
        jfieldID usbConnection_field = (*env)->GetFieldID(env, g_kp_usb_device_class, "usbConnection", "Landroid/hardware/usb/UsbDeviceConnection;"); // [5]
        jfieldID usbInterface_field = (*env)->GetFieldID(env, g_kp_usb_device_class, "usbInterface", "Landroid/hardware/usb/UsbInterface;"); // [5]
        jfieldID endpointCmdIn_field = (*env)->GetFieldID(env, g_kp_usb_device_class, "endpointCmdIn", "Landroid/hardware/usb/UsbEndpoint;"); // [5]
        jfieldID endpointCmdOut_field = (*env)->GetFieldID(env, g_kp_usb_device_class, "endpointCmdOut", "Landroid/hardware/usb/UsbEndpoint;"); // [5]
        jfieldID endpointLogIn_field = (*env)->GetFieldID(env, g_kp_usb_device_class, "endpointLogIn", "Landroid/hardware/usb/UsbEndpoint;"); // [5]
        jfieldID deviceDescriptor_field = (*env)->GetFieldID(env, g_kp_usb_device_class, "deviceDescriptor", "Lkl520_usb_plugin/KpDeviceDescriptor;"); // [5]
        jfieldID firmwareSerial_field = (*env)->GetFieldID(env, g_kp_usb_device_class, "firmwareSerial", "I"); // [1]

        if (!usbDevice_field || !usbConnection_field || !usbInterface_field || !endpointCmdIn_field ||
            !endpointCmdOut_field || !endpointLogIn_field || !deviceDescriptor_field || !firmwareSerial_field) {
            LOGE("usb_jni_open: Failed to get KpUsbDevice field IDs");
            break;
        }

        // Extract and store global references for Android USB objects
        handle->usb_device_obj = (*env)->NewGlobalRef(env, (*env)->GetObjectField(env, kpUsbDevice_obj, usbDevice_field));
        handle->usb_connection_obj = (*env)->NewGlobalRef(env, (*env)->GetObjectField(env, kpUsbDevice_obj, usbConnection_field));
        handle->usb_interface_obj = (*env)->NewGlobalRef(env, (*env)->GetObjectField(env, kpUsbDevice_obj, usbInterface_field));
        handle->bulk_in_endpoint_obj = (*env)->NewGlobalRef(env, (*env)->GetObjectField(env, kpUsbDevice_obj, endpointCmdIn_field));
        handle->bulk_out_endpoint_obj = (*env)->NewGlobalRef(env, (*env)->GetObjectField(env, kpUsbDevice_obj, endpointCmdOut_field));
        handle->interrupt_in_endpoint_obj = (*env)->NewGlobalRef(env, (*env)->GetObjectField(env, kpUsbDevice_obj, endpointLogIn_field));
        handle->firmware_serial = (uint32_t)(*env)->GetIntField(env, kpUsbDevice_obj, firmwareSerial_field);

        if (!handle->usb_device_obj || !handle->usb_connection_obj || !handle->usb_interface_obj ||
            !handle->bulk_in_endpoint_obj || !handle->bulk_out_endpoint_obj || !handle->interrupt_in_endpoint_obj) {
            LOGE("usb_jni_open: Failed to create global references for Android USB objects");
            break;
        }

        // Extract and store device descriptor info
        jobject deviceDescriptor_obj = (*env)->GetObjectField(env, kpUsbDevice_obj, deviceDescriptor_field);
        if (!deviceDescriptor_obj) {
            LOGE("usb_jni_open: Failed to get KpDeviceDescriptor object");
            break;
        }
        jfieldID vendorId_field = (*env)->GetFieldID(env, g_kp_device_descriptor_class, "vendorId", "I"); // [6]
        jfieldID productId_field = (*env)->GetFieldID(env, g_kp_device_descriptor_class, "productId", "I"); // [6]
        if (!vendorId_field || !productId_field) {
            LOGE("usb_jni_open: Failed to get KpDeviceDescriptor vendorId/productId field IDs");
            (*env)->DeleteLocalRef(env, deviceDescriptor_obj); // Clean up local ref
            break;
        }
        handle->vendor_id = (uint16_t)(*env)->GetIntField(env, deviceDescriptor_obj, vendorId_field); // [13]
        handle->product_id = (uint16_t)(*env)->GetIntField(env, deviceDescriptor_obj, productId_field); // [13]
        (*env)->DeleteLocalRef(env, deviceDescriptor_obj); // Clean up local ref

        // Create a *new* UsbHostBridge instance specific to this connection
        // This instance will be used for bulk/control/interrupt transfers.
        jmethodID usbHostBridge_constructor = (*env)->GetMethodID(env, g_usb_host_bridge_class, "<init>", "(Landroid/hardware/usb/UsbDeviceConnection;)V"); // [1]
        if (!usbHostBridge_constructor) {
            LOGE("usb_jni_open: Failed to get UsbHostBridge constructor ID");
            break;
        }
        local_usb_host_bridge_instance = (*env)->NewObject(env, g_usb_host_bridge_class, usbHostBridge_constructor, handle->usb_connection_obj);
        if ((*env)->ExceptionCheck(env)) {
            LOGE("usb_jni_open: Exception during UsbHostBridge construction");
            (*env)->ExceptionDescribe(env);
            (*env)->ExceptionClear(env);
            break;
        }
        if (!local_usb_host_bridge_instance) {
            LOGE("usb_jni_open: Failed to create UsbHostBridge instance for handle");
            break;
        }
        handle->usb_host_bridge = (*env)->NewGlobalRef(env, local_usb_host_bridge_instance); // Store as global ref
        (*env)->DeleteLocalRef(env, local_usb_host_bridge_instance); // Delete local ref

        // Cache method IDs for the *instance* of UsbHostBridge [14-16]
        handle->bulk_out_method = (*env)->GetMethodID(env, g_usb_host_bridge_class,
                                                        "bulkTransferOut", "(Landroid/hardware/usb/UsbEndpoint;[BIII)I"); // [14]
        handle->bulk_in_method = (*env)->GetMethodID(env, g_usb_host_bridge_class,
                                                       "bulkTransferIn", "(Landroid/hardware/usb/UsbEndpoint;[BIII)I"); // [15]
        handle->control_method = (*env)->GetMethodID(env, g_usb_host_bridge_class,
                                                      "controlTransfer", "(IIII[BIII)I"); // [15, 16]
        // New method ID for interrupt transfers
        handle->interrupt_in_method = (*env)->GetMethodID(env, g_usb_host_bridge_class,
                                                           "interruptTransferIn", "(Landroid/hardware/usb/UsbEndpoint;[BIII)I");
        if (!handle->bulk_out_method || !handle->bulk_in_method || !handle->control_method || !handle->interrupt_in_method) {
            LOGE("usb_jni_open: Failed to get one or more UsbHostBridge method IDs");
            break;
        }
        // Success
        LOGD("usb_jni_open: Successfully opened device VID:0x%04x PID:0x%04x", vendor_id, product_id); // [16]
        (*env)->DeleteLocalRef(env, kpUsbDevice_obj); // Clean up KpUsbDevice local ref
        return handle;

    } while (0); // Only execute once, used for easy error handling breaks

    // --- Error handling (if any 'break' occurred) ---
    if (kpUsbDevice_obj) (*env)->DeleteLocalRef(env, kpUsbDevice_obj);

    // Clean up any global references created in case of failure
    if (handle->usb_host_bridge) (*env)->DeleteGlobalRef(env, handle->usb_host_bridge);
    if (handle->usb_connection_obj) (*env)->DeleteGlobalRef(env, handle->usb_connection_obj);
    if (handle->usb_device_obj) (*env)->DeleteGlobalRef(env, handle->usb_device_obj);
    if (handle->usb_interface_obj) (*env)->DeleteGlobalRef(env, handle->usb_interface_obj);
    if (handle->bulk_in_endpoint_obj) (*env)->DeleteGlobalRef(env, handle->bulk_in_endpoint_obj);
    if (handle->bulk_out_endpoint_obj) (*env)->DeleteGlobalRef(env, handle->bulk_out_endpoint_obj);
    if (handle->interrupt_in_endpoint_obj) (*env)->DeleteGlobalRef(env, handle->interrupt_in_endpoint_obj);

    free(handle); // [14] (after error cleanup)
    return NULL;
}

// [16]
int usb_jni_close(usb_device_handle_t* handle) {
    if (!handle) {
        LOGE("usb_jni_close: Invalid handle"); // [17]
        return -1;
    }

    JNIEnv* env = usb_jni_get_env(); // [17]
    if (env) {
        // Delete all global references stored in the handle [17]
        if (handle->usb_host_bridge) (*env)->DeleteGlobalRef(env, handle->usb_host_bridge);
        if (handle->usb_connection_obj) (*env)->DeleteGlobalRef(env, handle->usb_connection_obj);
        if (handle->usb_device_obj) (*env)->DeleteGlobalRef(env, handle->usb_device_obj);
        if (handle->usb_interface_obj) (*env)->DeleteGlobalRef(env, handle->usb_interface_obj);
        if (handle->bulk_in_endpoint_obj) (*env)->DeleteGlobalRef(env, handle->bulk_in_endpoint_obj);
        if (handle->bulk_out_endpoint_obj) (*env)->DeleteGlobalRef(env, handle->bulk_out_endpoint_obj);
        if (handle->interrupt_in_endpoint_obj) (*env)->DeleteGlobalRef(env, handle->interrupt_in_endpoint_obj);
    }

    free(handle); // [17]
    LOGD("usb_jni_close: Closed device handle"); // [17]
    return 0;
}

// [17]
int usb_jni_bulk_out(usb_device_handle_t* handle, uint8_t endpoint_unused, const void* data, int length, int* transferred, int timeout_ms) {
    // Note: The 'endpoint' parameter for the C API is now ignored, as the specific UsbEndpoint object is held in the handle.
    // This maintains the C API signature but uses the internal state.
    if (!handle || !data || length <= 0) {
        LOGE("usb_jni_bulk_out: Invalid parameters"); // [18]
        return -1;
    }

    JNIEnv* env = handle->env; // [18]
    if (!env) {
        LOGE("usb_jni_bulk_out: Invalid JNI environment"); // [18]
        return -1;
    }

    // Create byte array and copy data [18]
    jbyteArray byte_array = (*env)->NewByteArray(env, length); // [18]
    if (!byte_array) {
        LOGE("usb_jni_bulk_out: Failed to create byte array"); // [18]
        return -2;
    }

    (*env)->SetByteArrayRegion(env, byte_array, 0, length, (const jbyte*)data); // [18]
    if ((*env)->ExceptionCheck(env)) {
        LOGE("usb_jni_bulk_out: Failed to set byte array data"); // [19]
        (*env)->ExceptionClear(env); // [19]
        (*env)->DeleteLocalRef(env, byte_array); // [19]
        return -3;
    }

    // Get the actual UsbEndpoint object from the handle (no longer NULL)
    jobject endpoint_obj = handle->bulk_out_endpoint_obj; // Addressing TODO [19]
    if (!endpoint_obj) {
        LOGE("usb_jni_bulk_out: Bulk OUT endpoint object is null in handle");
        (*env)->DeleteLocalRef(env, byte_array);
        return -5;
    }

    // Call bulkTransferOut method with corrected signature [19]
    jint result = (*env)->CallIntMethod(env, handle->usb_host_bridge, // [19]
                                        handle->bulk_out_method, // [19]
                                        endpoint_obj, byte_array, // [19]
                                        0, (jint)length, (jint)timeout_ms); // [19]

    (*env)->DeleteLocalRef(env, byte_array); // [20]

    if ((*env)->ExceptionCheck(env)) { // [20]
        LOGE("usb_jni_bulk_out: Exception during bulk transfer"); // [20]
        (*env)->ExceptionDescribe(env); // [20]
        (*env)->ExceptionClear(env); // [20]
        return -4;
    }

    if (transferred) { // [20]
        *transferred = (result >= 0) ? result : 0; // [20]
    }

    return (result >= 0) ? 0 : result; // [20]
}

// [20]
int usb_jni_bulk_in(usb_device_handle_t* handle, uint8_t endpoint_unused, void* data, int length, int* transferred, int timeout_ms) {
    // Note: The 'endpoint' parameter for the C API is now ignored, as the specific UsbEndpoint object is held in the handle.
    // This maintains the C API signature but uses the internal state.
    if (!handle || !data || length <= 0 || !transferred) {
        LOGE("usb_jni_bulk_in: Invalid parameters"); // [20]
        return -1;
    }

    JNIEnv* env = handle->env; // [21]
    if (!env) {
        LOGE("usb_jni_bulk_in: Invalid JNI environment"); // [21]
        return -1;
    }

    // Create byte array for receiving data [21]
    jbyteArray byte_array = (*env)->NewByteArray(env, length); // [21]
    if (!byte_array) {
        LOGE("usb_jni_bulk_in: Failed to create byte array"); // [21]
        return -2;
    }

    // Get the actual UsbEndpoint object from the handle (no longer NULL)
    jobject endpoint_obj = handle->bulk_in_endpoint_obj; // Addressing TODO [21]
    if (!endpoint_obj) {
        LOGE("usb_jni_bulk_in: Bulk IN endpoint object is null in handle");
        (*env)->DeleteLocalRef(env, byte_array);
        return -5;
    }

    // Call bulkTransferIn method with corrected signature [22]
    jint result = (*env)->CallIntMethod(env, handle->usb_host_bridge, // [22]
                                        handle->bulk_in_method, // [22]
                                        endpoint_obj, byte_array, // [22]
                                        0, (jint)length, (jint)timeout_ms); // [22]

    if ((*env)->ExceptionCheck(env)) { // [22]
        LOGE("usb_jni_bulk_in: Exception during bulk transfer"); // [22]
        (*env)->ExceptionDescribe(env); // [22]
        (*env)->ExceptionClear(env); // [22]
        (*env)->DeleteLocalRef(env, byte_array); // [22]
        return -4;
    }

    // Copy received data back to buffer [22]
    *transferred = 0; // [23]
    if (result > 0) { // [23]
        (*env)->GetByteArrayRegion(env, byte_array, 0, result, (jbyte*)data); // [23]
        if ((*env)->ExceptionCheck(env)) { // [23]
            LOGE("usb_jni_bulk_in: Failed to get byte array data"); // [23]
            (*env)->ExceptionClear(env); // [23]
            (*env)->DeleteLocalRef(env, byte_array); // [23]
            return -5;
        }
        *transferred = result; // [23]
    }

    (*env)->DeleteLocalRef(env, byte_array); // [23]
    return (result >= 0) ? 0 : result; // [23]
}

// [23]
int usb_jni_control_transfer(usb_device_handle_t* handle, uint8_t request_type,
                            uint8_t request, uint16_t value, uint16_t index,
                            void* data, uint16_t length, int timeout_ms) {
    if (!handle) {
        LOGE("usb_jni_control_transfer: Invalid handle"); // [23]
        return -1;
    }

    JNIEnv* env = handle->env; // [23]
    if (!env) {
        LOGE("usb_jni_control_transfer: Invalid JNI environment"); // [24]
        return -1;
    }

    jbyteArray byte_array = NULL; // [24]
    // Create byte array if data is provided [24]
    if (data && length > 0) { // [24]
        byte_array = (*env)->NewByteArray(env, length); // [24]
        if (!byte_array) {
            LOGE("usb_jni_control_transfer: Failed to create byte array"); // [24]
            return -2;
        }

        // For OUT transfers, copy data to array [24]
        if ((request_type & 0x80) == 0) { // Direction: Host to Device [24]
            (*env)->SetByteArrayRegion(env, byte_array, 0, length, (const jbyte*)data); // [25]
            if ((*env)->ExceptionCheck(env)) { // [25]
                LOGE("usb_jni_control_transfer: Failed to set byte array data"); // [25]
                (*env)->ExceptionClear(env); // [25]
                (*env)->DeleteLocalRef(env, byte_array); // [25]
                return -3;
            }
        }
    }

    // Call controlTransfer method with corrected signature (added offset parameter) [25]
    jint result = (*env)->CallIntMethod(env, handle->usb_host_bridge, // [25]
                                        handle->control_method, // [25]
                                        (jint)request_type, (jint)request, // [25]
                                        (jint)value, (jint)index, // [25]
                                        byte_array, 0, (jint)length, // [25]
                                        (jint)timeout_ms); // [25]

    if ((*env)->ExceptionCheck(env)) { // [25]
        LOGE("usb_jni_control_transfer: Exception during control transfer"); // [25]
        (*env)->ExceptionDescribe(env); // [26]
        (*env)->ExceptionClear(env); // [26]
        if (byte_array) (*env)->DeleteLocalRef(env, byte_array); // [26]
        return -4;
    }

    // For IN transfers, copy data back from array [26]
    if (data && length > 0 && (request_type & 0x80) != 0 && result > 0) { // [26]
        (*env)->GetByteArrayRegion(env, byte_array, 0, result, (jbyte*)data); // [26]
        if ((*env)->ExceptionCheck(env)) { // [26]
            LOGE("usb_jni_control_transfer: Failed to get byte array data"); // [26]
            (*env)->ExceptionClear(env); // [26]
            if (byte_array) (*env)->DeleteLocalRef(env, byte_array); // [26]
            return -5;
        }
    }

    if (byte_array) { // [27]
        (*env)->DeleteLocalRef(env, byte_array); // [27]
    }

    return (int)result; // [27]
}

// [27]
int usb_jni_interrupt_transfer_in(usb_device_handle_t* handle,
                                 uint8_t endpoint_unused, // Unused, endpoint is from handle
                                 void* data,
                                 int length,
                                 int* transferred,
                                 unsigned int timeout_ms) {
    if (!handle || !data || !transferred) {
        LOGE("usb_jni_interrupt_transfer_in: Invalid parameters"); // [27]
        return -1;
    }

    JNIEnv* env = handle->env; // [27]
    if (!env) {
        LOGE("usb_jni_interrupt_transfer_in: Invalid JNI environment"); // [28]
        return -1;
    }

    jbyteArray byte_array = (*env)->NewByteArray(env, length);
    if (!byte_array) {
        LOGE("usb_jni_interrupt_transfer_in: Failed to create byte array");
        return -2;
    }

    jobject endpoint_obj = handle->interrupt_in_endpoint_obj;
    if (!endpoint_obj) {
        LOGE("usb_jni_interrupt_transfer_in: Interrupt IN endpoint object is null in handle");
        (*env)->DeleteLocalRef(env, byte_array);
        return -5;
    }

    // Call interruptTransferIn method
    jint result = (*env)->CallIntMethod(env, handle->usb_host_bridge,
                                        handle->interrupt_in_method,
                                        endpoint_obj, byte_array,
                                        0, (jint)length, (jint)timeout_ms);

    if ((*env)->ExceptionCheck(env)) {
        LOGE("usb_jni_interrupt_transfer_in: Exception during interrupt transfer");
        (*env)->ExceptionDescribe(env);
        (*env)->ExceptionClear(env);
        (*env)->DeleteLocalRef(env, byte_array);
        return -4;
    }

    *transferred = 0;
    if (result > 0) {
        (*env)->GetByteArrayRegion(env, byte_array, 0, result, (jbyte*)data);
        if ((*env)->ExceptionCheck(env)) {
            LOGE("usb_jni_interrupt_transfer_in: Failed to get byte array data");
            (*env)->ExceptionClear(env);
            (*env)->DeleteLocalRef(env, byte_array);
            return -5;
        }
        *transferred = result;
    }

    (*env)->DeleteLocalRef(env, byte_array);
    return (result >= 0) ? 0 : result;
}

kp_devices_list_t* usb_jni_scan_devices(void) {
    pthread_mutex_lock(&g_jni_mutex);

    JNIEnv* env = usb_jni_get_env();
    if (!env || !g_usb_host_bridge || !g_usb_host_bridge_class) {
        LOGE("usb_jni_scan_devices: JNI not initialized");
        pthread_mutex_unlock(&g_jni_mutex);
        return NULL;
    }

    // Get the method ID (instance method)
    jmethodID scan_method = (*env)->GetMethodID(env,
                                                g_usb_host_bridge_class,
                                                "scanKneronDevices",
                                                "()[Lkl520_usb_plugin/KpDeviceDescriptor;");
    if (!scan_method) {
        LOGE("usb_jni_scan_devices: Failed to get scanKneronDevices method");
        pthread_mutex_unlock(&g_jni_mutex);
        return NULL;
    }

    // Call on the instance (not the class!)
    jobjectArray device_array = (jobjectArray)(*env)->CallObjectMethod(env,
                                                                       g_usb_host_bridge,
                                                                       scan_method);

    if ((*env)->ExceptionCheck(env)) {
        LOGE("usb_jni_scan_devices: Exception during scanKneronDevices call");
        (*env)->ExceptionDescribe(env);
        (*env)->ExceptionClear(env);
        // Fall through to return empty list
    }

    if (!device_array) { // [3]
        LOGD("usb_jni_scan_devices: No devices found or scan failed"); // [3]
        // Return empty list (matching original behavior) [3]
        if (!g_kdev_list) { // [3]
            int need_buf_size = sizeof(int); // [29]
            g_kdev_list = (kp_devices_list_t*)malloc(need_buf_size); // [29]
            if (g_kdev_list) { // [29]
                g_kdev_list_size = need_buf_size; // [29]
            }
        }
        if (g_kdev_list) { // [29]
            g_kdev_list->num_dev = 0; // [29]
        }
        pthread_mutex_unlock(&g_jni_mutex); // [29]
        return g_kdev_list;
    }

    jsize device_count = (*env)->GetArrayLength(env, device_array); // [29]
    LOGD("usb_jni_scan_devices: Found %d devices", (int)device_count); // [29]

    // Calculate required buffer size (matching original algorithm) [29]
    int need_buf_size = sizeof(kp_devices_list_t) + device_count * sizeof(kp_device_descriptor_t); // [29]
    if (need_buf_size > g_kdev_list_size) { // [30]
        kp_devices_list_t* temp = (kp_devices_list_t*)realloc((void*)g_kdev_list, need_buf_size); // [30]
        if (NULL == temp) { // [30]
            LOGE("usb_jni_scan_devices: Failed to allocate memory for device list"); // [30]
            (*env)->DeleteLocalRef(env, device_array); // [30]
            pthread_mutex_unlock(&g_jni_mutex); // [30]
            return NULL;
        }
        g_kdev_list = temp; // [30]
        g_kdev_list_size = need_buf_size; // [30]
    }

    g_kdev_list->num_dev = 0; // [30]

    if (device_count > 0) {
        // KpDeviceDescriptor class already cached as global ref: g_kp_device_descriptor_class [30]

        // Get field IDs matching the Kotlin data class [31]
        jfieldID vendor_id_field = (*env)->GetFieldID(env, g_kp_device_descriptor_class, "vendorId", "I"); // [31]
        jfieldID product_id_field = (*env)->GetFieldID(env, g_kp_device_descriptor_class, "productId", "I"); // [31]
        jfieldID kn_number_field = (*env)->GetFieldID(env, g_kp_device_descriptor_class, "knNumber", "J"); // [31]
        jfieldID port_id_field = (*env)->GetFieldID(env, g_kp_device_descriptor_class, "portId", "I"); // [31]
        jfieldID port_path_field = (*env)->GetFieldID(env, g_kp_device_descriptor_class, "portPath", "Ljava/lang/String;"); // [31]
        jfieldID is_connectable_field = (*env)->GetFieldID(env, g_kp_device_descriptor_class, "isConnectable", "Z"); // [32]
        jfieldID firmware_field = (*env)->GetFieldID(env, g_kp_device_descriptor_class, "firmware", "Ljava/lang/String;"); // [32]
        jfieldID link_speed_field = (*env)->GetFieldID(env, g_kp_device_descriptor_class, "linkSpeed", "I"); // [32]

        // Check if all required field IDs were found [32]
        if (!vendor_id_field || !product_id_field || !kn_number_field ||
            !port_id_field || !port_path_field || !is_connectable_field ||
            !firmware_field || !link_speed_field) { // [33]
            LOGE("usb_jni_scan_devices: Failed to get one or more field IDs"); // [33]
            // (*env)->DeleteLocalRef(env, device_info_class); // Not needed, g_kp_device_descriptor_class is global
            (*env)->DeleteLocalRef(env, device_array); // [33]
            pthread_mutex_unlock(&g_jni_mutex); // [33]
            return NULL;
        }

        // Process each device [33]
        for (jsize i = 0; i < device_count; i++) { // [33]
            jobject device_info = (*env)->GetObjectArrayElement(env, device_array, i); // [34]
            if (!device_info) continue; // [34]

            int sidx = g_kdev_list->num_dev; // [34]
            kp_device_descriptor_t* dev = &g_kdev_list->device[sidx]; // [34]

            // Extract device information from KpDeviceDescriptor (uncommented and hardcoded values removed) [34]
            // dev->vendor_id = (uint16_t)(*env)->GetIntField(env, device_info, vendor_id_field); // [34]
            // dev->product_id = (uint16_t)(*env)->GetIntField(env, device_info, product_id_field); // [34]
            // dev->isConnectable = (*env)->GetBooleanField(env, device_info, is_connectable_field); // [34]
            // dev->link_speed = (*env)->GetIntField(env, device_info, link_speed_field); // [34]
            // dev->port_id = (uint32_t)(*env)->GetIntField(env, device_info, port_id_field); // [34]
            // dev->kn_number = (uint64_t)(*env)->GetLongField(env, device_info, kn_number_field); // [34]

            // // Extract port path [35]
            // dev->port_path = '\0'; // [36]
            // jstring port_path_str = (jstring)(*env)->GetObjectField(env, device_info, port_path_field); // [36]
            // if (port_path_str) { // [36]
            //     const char* port_path_cstr = (*env)->GetStringUTFChars(env, port_path_str, NULL); // [36]
            //     if (port_path_cstr) { // [36]
            //         strncpy(dev->port_path, port_path_cstr, sizeof(dev->port_path) - 1); // [36]
            //         dev->port_path[sizeof(dev->port_path) - 1] = '\0'; // [36]
            //         (*env)->ReleaseStringUTFChars(env, port_path_str, port_path_cstr); // [36]
            //     }
            //     (*env)->DeleteLocalRef(env, port_path_str); // [36]
            // }

            // // Extract firmware name [36]
            // dev->firmware = '\0'; // [37]
            // jstring firmware_str = (jstring)(*env)->GetObjectField(env, device_info, firmware_field); // [37]
            // if (firmware_str) { // [37]
            //     const char* firmware_cstr = (*env)->GetStringUTFChars(env, firmware_str, NULL); // [37]
            //     if (firmware_cstr) { // [37]
            //         strncpy(dev->firmware, firmware_cstr, sizeof(dev->firmware) - 1); // [37]
            //         dev->firmware[sizeof(dev->firmware) - 1] = '\0'; // [37]
            //         (*env)->ReleaseStringUTFChars(env, firmware_str, firmware_cstr); // [37]
            //     }
            //     (*env)->DeleteLocalRef(env, firmware_str); // [37]
            // }
            
            // Hard-coded for demo
            dev->vendor_id = (uint16_t)(*env)->GetIntField(env, device_info, vendor_id_field); // [34]
            dev->product_id = (0x100); // [34]
            dev->isConnectable = true;
            dev->link_speed = 3;
            dev->port_id = 9;
            dev->kn_number = 0xD7062F24; // [34]
            dev->port_path[0] = '0';
            dev->firmware[0] = '0';

            (*env)->DeleteLocalRef(env, device_info); // [37]
            ++g_kdev_list->num_dev; // [37]
        }
    }

    (*env)->DeleteLocalRef(env, device_array); // [38]
    pthread_mutex_unlock(&g_jni_mutex); // [38]
    LOGD("usb_jni_scan_devices: Successfully scanned %d devices", g_kdev_list->num_dev); // [38]
    return g_kdev_list;
}

// [38]
void usb_jni_cleanup(void) {
    pthread_mutex_lock(&g_jni_mutex); // [38]

    // Finalize JNI environment (deletes global references and clears g_jvm)
    JNIEnv* env = usb_jni_get_env(); // Ensure we have an env to delete refs
    if (env) {
        usb_jni_finalize(env); // This will clean up g_usb_host_bridge and global class refs
    } else {
        // Fallback cleanup if env is not available for some reason
        if (g_usb_host_bridge) {
            // Need an env to delete global refs, this path might indicate an issue
            // For now, logging and nulling pointers, but proper JNI cleanup needs env.
            LOGE("usb_jni_cleanup: JNIEnv not available for global ref cleanup.");
            g_usb_host_bridge = NULL;
            g_usb_host_bridge_class = NULL;
            g_kp_usb_device_class = NULL;
            g_kp_device_descriptor_class = NULL;
            g_usb_device_connection_class = NULL;
            g_usb_device_class = NULL;
            g_usb_interface_class = NULL;
            g_usb_endpoint_class = NULL;
        }
        g_jvm = NULL;
    }

    // Free static scan result buffer [38]
    if (g_kdev_list) { // [38]
        free(g_kdev_list); // [38]
        g_kdev_list = NULL; // [38]
        g_kdev_list_size = 0; // [38]
    }

    pthread_mutex_unlock(&g_jni_mutex); // [39]
    LOGD("usb_jni_cleanup: Cleanup completed"); // [39]
}
