#include "kp_usb_jni.h"
#include <stdlib.h>
#include <string.h>
#include <android/log.h>
#include <pthread.h>

#define LOG_TAG "KP_USB_TRANSPORT"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// Structure to hold USB device handle information
struct usb_device_handle {
    JNIEnv* env;
    jobject usb_host_bridge;     // Reference to UsbHostBridge object
    jobject usb_connection;      // Reference to UsbDeviceConnection object
    jclass bridge_class;         // Cached class reference
    jmethodID bulk_out_method;   // Cached method ID for bulk out
    jmethodID bulk_in_method;    // Cached method ID for bulk in  
    jmethodID control_method;    // Cached method ID for control transfer
    uint16_t vendor_id;
    uint16_t product_id;
    uint8_t endpoint_bulk_in;
    uint8_t endpoint_bulk_out;
};

// Global state
static JavaVM* g_jvm = NULL;
static jobject g_usb_host_bridge = NULL;
static pthread_mutex_t g_jni_mutex = PTHREAD_MUTEX_INITIALIZER;

// Static buffer for device list (matching original libusb implementation pattern)
static kp_devices_list_t* g_kdev_list = NULL;
static int g_kdev_list_size = 0;

// ---------------------------------------------------------------------------
// Helper utilities
// ---------------------------------------------------------------------------

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

/**
 * @brief Generate port_id from port_path (matching original algorithm)
 */
static uint32_t generate_port_id_from_path(const char* port_path) {
    if (!port_path || port_path[0] == '\0') {
        return 0;
    }
    
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

int usb_jni_initialize(JNIEnv* env, jobject usb_host_bridge) {
    if (!env || !usb_host_bridge) {
        LOGE("usb_jni_initialize: Invalid parameters");
        return -1;
    }
    
    pthread_mutex_lock(&g_jni_mutex);
    
    // Store global JavaVM
    if ((*env)->GetJavaVM(env, &g_jvm) != JNI_OK) {
        LOGE("usb_jni_initialize: Failed to get JavaVM");
        pthread_mutex_unlock(&g_jni_mutex);
        return -1;
    }
    
    // Clean up existing global reference if any
    if (g_usb_host_bridge) {
        (*env)->DeleteGlobalRef(env, g_usb_host_bridge);
    }
    
    // Create global reference to prevent GC
    g_usb_host_bridge = (*env)->NewGlobalRef(env, usb_host_bridge);
    if (!g_usb_host_bridge) {
        LOGE("usb_jni_initialize: Failed to create global reference");
        g_jvm = NULL;
        pthread_mutex_unlock(&g_jni_mutex);
        return -1;
    }
    
    pthread_mutex_unlock(&g_jni_mutex);
    LOGD("usb_jni_initialize: Initialization successful");
    return 0;
}

int usb_jni_finalize(JNIEnv* env) {
    if (!env) return -1;

    pthread_mutex_lock(&g_jni_mutex);
    
    if (g_usb_host_bridge) {
        (*env)->DeleteGlobalRef(env, g_usb_host_bridge);
        g_usb_host_bridge = NULL;
    }

    g_jvm = NULL;
    
    pthread_mutex_unlock(&g_jni_mutex);
    LOGD("usb_jni_finalize: Finalized USB JNI");
    return 0;
}

usb_device_handle_t* usb_jni_open(uint16_t vendor_id, uint16_t product_id) {
    if (!g_jvm || !g_usb_host_bridge) {
        LOGE("usb_jni_open: Transport not initialized");
        return NULL;
    }

    JNIEnv* env = usb_jni_get_env();
    if (!env) {
        LOGE("usb_jni_open: Failed to get JNI environment");
        return NULL;
    }

    // Allocate device handle
    usb_device_handle_t* handle = (usb_device_handle_t*)malloc(sizeof(usb_device_handle_t));
    if (!handle) {
        LOGE("usb_jni_open: Failed to allocate memory");
        return NULL;
    }

    memset(handle, 0, sizeof(usb_device_handle_t));
    handle->env = env;
    handle->vendor_id = vendor_id;
    handle->product_id = product_id;
    handle->usb_host_bridge = g_usb_host_bridge;

    // Get the UsbHostBridge class
    handle->bridge_class = (*env)->GetObjectClass(env, g_usb_host_bridge);
    if (!handle->bridge_class) {
        LOGE("usb_jni_open: Failed to get UsbHostBridge class");
        free(handle);
        return NULL;
    }

    // Cache method IDs - updated signatures to match Kotlin implementation
    handle->bulk_out_method = (*env)->GetMethodID(env, handle->bridge_class,
        "bulkTransferOut", "(Landroid/hardware/usb/UsbEndpoint;[BIII)I");
    if (!handle->bulk_out_method) {
        LOGE("usb_jni_open: Failed to get bulkTransferOut method");
        (*env)->DeleteLocalRef(env, handle->bridge_class);
        free(handle);
        return NULL;
    }

    handle->bulk_in_method = (*env)->GetMethodID(env, handle->bridge_class,
        "bulkTransferIn", "(Landroid/hardware/usb/UsbEndpoint;[BIII)I");
    if (!handle->bulk_in_method) {
        LOGE("usb_jni_open: Failed to get bulkTransferIn method");
        (*env)->DeleteLocalRef(env, handle->bridge_class);
        free(handle);
        return NULL;
    }

    handle->control_method = (*env)->GetMethodID(env, handle->bridge_class,
        "controlTransfer", "(IIII[BIII)I");
    if (!handle->control_method) {
        LOGE("usb_jni_open: Failed to get controlTransfer method");
        (*env)->DeleteLocalRef(env, handle->bridge_class);
        free(handle);
        return NULL;
    }

    // Set default endpoints (these would typically be determined during device enumeration)
    handle->endpoint_bulk_in = 0x81;   // IN endpoint 1
    handle->endpoint_bulk_out = 0x01;  // OUT endpoint 1

    LOGD("usb_jni_open: Successfully opened device VID:0x%04x PID:0x%04x", 
         vendor_id, product_id);
    return handle;
}

int usb_jni_close(usb_device_handle_t* handle) {
    if (!handle) {
        LOGE("usb_jni_close: Invalid handle");
        return -1;
    }

    JNIEnv* env = usb_jni_get_env();
    if (env) {
        if (handle->bridge_class) {
            (*env)->DeleteLocalRef(env, handle->bridge_class);
        }
        if (handle->usb_connection) {
            (*env)->DeleteGlobalRef(env, handle->usb_connection);
        }
    }

    free(handle);
    LOGD("usb_jni_close: Closed device handle");
    return 0;
}

int usb_jni_bulk_out(usb_device_handle_t* handle, uint8_t endpoint, const void* data, int length, int* transferred, int timeout_ms) {
    if (!handle || !data || length <= 0) {
        LOGE("usb_jni_bulk_out: Invalid parameters");
        return -1;
    }

    JNIEnv* env = handle->env;
    if (!env) {
        LOGE("usb_jni_bulk_out: Invalid JNI environment");
        return -1;
    }
    
    // Create byte array and copy data
    jbyteArray byte_array = (*env)->NewByteArray(env, length);
    if (!byte_array) {
        LOGE("usb_jni_bulk_out: Failed to create byte array");
        return -2;
    }

    (*env)->SetByteArrayRegion(env, byte_array, 0, length, (const jbyte*)data);
    if ((*env)->ExceptionCheck(env)) {
        LOGE("usb_jni_bulk_out: Failed to set byte array data");
        (*env)->ExceptionClear(env);
        (*env)->DeleteLocalRef(env, byte_array);
        return -3;
    }

    // TODO: Need to get the actual UsbEndpoint object for this endpoint
    // This requires additional JNI calls to retrieve the endpoint from the device
    jobject endpoint_obj = NULL; // This needs to be implemented
    
    // Call bulkTransferOut method with corrected signature
    jint result = (*env)->CallIntMethod(env, handle->usb_host_bridge, 
                                       handle->bulk_out_method,
                                       endpoint_obj, byte_array, 
                                       0, (jint)length, (jint)timeout_ms);

    (*env)->DeleteLocalRef(env, byte_array);

    if ((*env)->ExceptionCheck(env)) {
        LOGE("usb_jni_bulk_out: Exception during bulk transfer");
        (*env)->ExceptionDescribe(env);
        (*env)->ExceptionClear(env);
        return -4;
    }

    if (transferred) {
        *transferred = (result >= 0) ? result : 0;
    }

    return (result >= 0) ? 0 : result;
}

int usb_jni_bulk_in(usb_device_handle_t* handle, uint8_t endpoint, void* data, int length, int* transferred, int timeout_ms) {
    if (!handle || !data || length <= 0 || !transferred) {
        LOGE("usb_jni_bulk_in: Invalid parameters");
        return -1;
    }

    JNIEnv* env = handle->env;
    if (!env) {
        LOGE("usb_jni_bulk_in: Invalid JNI environment");
        return -1;
    }
    
    // Create byte array for receiving data
    jbyteArray byte_array = (*env)->NewByteArray(env, length);
    if (!byte_array) {
        LOGE("usb_jni_bulk_in: Failed to create byte array");
        return -2;
    }

    // TODO: Need to get the actual UsbEndpoint object for this endpoint
    jobject endpoint_obj = NULL; // This needs to be implemented

    // Call bulkTransferIn method with corrected signature
    jint result = (*env)->CallIntMethod(env, handle->usb_host_bridge,
                                       handle->bulk_in_method,
                                       endpoint_obj, byte_array,
                                       0, (jint)length, (jint)timeout_ms);

    if ((*env)->ExceptionCheck(env)) {
        LOGE("usb_jni_bulk_in: Exception during bulk transfer");
        (*env)->ExceptionDescribe(env);
        (*env)->ExceptionClear(env);
        (*env)->DeleteLocalRef(env, byte_array);
        return -4;
    }

    // Copy received data back to buffer
    *transferred = 0;
    if (result > 0) {
        (*env)->GetByteArrayRegion(env, byte_array, 0, result, (jbyte*)data);
        if ((*env)->ExceptionCheck(env)) {
            LOGE("usb_jni_bulk_in: Failed to get byte array data");
            (*env)->ExceptionClear(env);
            (*env)->DeleteLocalRef(env, byte_array);
            return -5;
        }
        *transferred = result;
    }

    (*env)->DeleteLocalRef(env, byte_array);
    return (result >= 0) ? 0 : result;
}

int usb_jni_control_transfer(usb_device_handle_t* handle, uint8_t request_type,
                             uint8_t request, uint16_t value, uint16_t index,
                             void* data, uint16_t length, int timeout_ms) {
    if (!handle) {
        LOGE("usb_jni_control_transfer: Invalid handle");
        return -1;
    }

    JNIEnv* env = handle->env;
    if (!env) {
        LOGE("usb_jni_control_transfer: Invalid JNI environment");
        return -1;
    }

    jbyteArray byte_array = NULL;

    // Create byte array if data is provided
    if (data && length > 0) {
        byte_array = (*env)->NewByteArray(env, length);
        if (!byte_array) {
            LOGE("usb_jni_control_transfer: Failed to create byte array");
            return -2;
        }

        // For OUT transfers, copy data to array
        if ((request_type & 0x80) == 0) {  // Direction: Host to Device
            (*env)->SetByteArrayRegion(env, byte_array, 0, length, (const jbyte*)data);
            if ((*env)->ExceptionCheck(env)) {
                LOGE("usb_jni_control_transfer: Failed to set byte array data");
                (*env)->ExceptionClear(env);
                (*env)->DeleteLocalRef(env, byte_array);
                return -3;
            }
        }
    }

    // Call controlTransfer method with corrected signature (added offset parameter)
    jint result = (*env)->CallIntMethod(env, handle->usb_host_bridge,
                                       handle->control_method,
                                       (jint)request_type, (jint)request,
                                       (jint)value, (jint)index,
                                       byte_array, 0, (jint)length,
                                       (jint)timeout_ms);

    if ((*env)->ExceptionCheck(env)) {
        LOGE("usb_jni_control_transfer: Exception during control transfer");
        (*env)->ExceptionDescribe(env);
        (*env)->ExceptionClear(env);
        if (byte_array) (*env)->DeleteLocalRef(env, byte_array);
        return -4;
    }

    // For IN transfers, copy data back from array
    if (data && length > 0 && (request_type & 0x80) != 0 && result > 0) {
        (*env)->GetByteArrayRegion(env, byte_array, 0, result, (jbyte*)data);
        if ((*env)->ExceptionCheck(env)) {
            LOGE("usb_jni_control_transfer: Failed to get byte array data");
            (*env)->ExceptionClear(env);
            if (byte_array) (*env)->DeleteLocalRef(env, byte_array);
            return -5;
        }
    }

    if (byte_array) {
        (*env)->DeleteLocalRef(env, byte_array);
    }

    return (int)result;
}

int usb_jni_interrupt_transfer_in(usb_device_handle_t* handle,
                                  uint8_t endpoint,
                                  void* data,
                                  int length,
                                  int* transferred,
                                  unsigned int timeout_ms)
{
    if (!handle || !data || !transferred) {
        LOGE("usb_jni_interrupt_transfer_in: Invalid parameters");
        return -1;
    }

    JNIEnv* env = handle->env;
    if (!env) {
        LOGE("usb_jni_interrupt_transfer_in: Invalid JNI environment");
        return -1;
    }

    // TODO: This function references fields that don't exist in the handle structure
    // Need to implement endpoint mapping and proper method resolution
    // The current implementation references handle->epMap and handle->bridgeObj
    // which are not defined in the structure above
    
    LOGE("usb_jni_interrupt_transfer_in: Function not fully implemented - missing endpoint mapping");
    return -1;
}

kp_devices_list_t* usb_jni_scan_devices(void) {
    pthread_mutex_lock(&g_jni_mutex);
    
    JNIEnv* env = usb_jni_get_env();
    if (!env || !g_usb_host_bridge) {
        LOGE("usb_jni_scan_devices: JNI not initialized");
        pthread_mutex_unlock(&g_jni_mutex);
        return NULL;
    }

    // Get the UsbHostBridge class
    jclass bridge_class = (*env)->GetObjectClass(env, g_usb_host_bridge);
    if (!bridge_class) {
        LOGE("usb_jni_scan_devices: Failed to get bridge class");
        pthread_mutex_unlock(&g_jni_mutex);
        return NULL;
    }

    // Get the scanKneronDevices method
    jmethodID scan_method = (*env)->GetMethodID(env, bridge_class, 
                                               "scanKneronDevices", 
                                               "()[Lkl520_usb_plugin/KpDeviceDescriptor;");
    if (!scan_method) {
        LOGE("usb_jni_scan_devices: Failed to get scanKneronDevices method");
        (*env)->DeleteLocalRef(env, bridge_class);
        pthread_mutex_unlock(&g_jni_mutex);
        return NULL;
    }

    // Call the scan method
    jobjectArray device_array = (jobjectArray)(*env)->CallObjectMethod(env, 
                                                                       g_usb_host_bridge, 
                                                                       scan_method);
    (*env)->DeleteLocalRef(env, bridge_class);
    
    if (!device_array) {
        LOGD("usb_jni_scan_devices: No devices found or scan failed");
        // Return empty list (matching original behavior)
        if (!g_kdev_list) {
            int need_buf_size = sizeof(int);
            g_kdev_list = (kp_devices_list_t*)malloc(need_buf_size);
            if (g_kdev_list) {
                g_kdev_list_size = need_buf_size;
            }
        }
        if (g_kdev_list) {
            g_kdev_list->num_dev = 0;
        }
        pthread_mutex_unlock(&g_jni_mutex);
        return g_kdev_list;
    }

    jsize device_count = (*env)->GetArrayLength(env, device_array);
    LOGD("usb_jni_scan_devices: Found %d devices", (int)device_count);

    // Calculate required buffer size (matching original algorithm)
    int need_buf_size = sizeof(int) + device_count * sizeof(kp_device_descriptor_t);

    if (need_buf_size > g_kdev_list_size) {
        kp_devices_list_t* temp = (kp_devices_list_t*)realloc((void*)g_kdev_list, need_buf_size);
        if (NULL == temp) {
            LOGE("usb_jni_scan_devices: Failed to allocate memory for device list");
            (*env)->DeleteLocalRef(env, device_array);
            pthread_mutex_unlock(&g_jni_mutex);
            return NULL;
        }
        g_kdev_list = temp;
        g_kdev_list_size = need_buf_size;
    }

    g_kdev_list->num_dev = 0;

    if (device_count > 0) {
        // Get KpDeviceDescriptor class and field IDs
        jclass device_info_class = (*env)->FindClass(env, "kl520_usb_plugin/KpDeviceDescriptor");
        if (!device_info_class) {
            LOGE("usb_jni_scan_devices: Failed to find KpDeviceDescriptor class");
            (*env)->DeleteLocalRef(env, device_array);
            pthread_mutex_unlock(&g_jni_mutex);
            return NULL;
        }

        // Get field IDs matching the Kotlin data class
        jfieldID vendor_id_field = (*env)->GetFieldID(env, device_info_class, "vendorId", "I");
        jfieldID product_id_field = (*env)->GetFieldID(env, device_info_class, "productId", "I");
        jfieldID kn_number_field = (*env)->GetFieldID(env, device_info_class, "knNumber", "J");
        jfieldID port_id_field = (*env)->GetFieldID(env, device_info_class, "portId", "I");
        jfieldID port_path_field = (*env)->GetFieldID(env, device_info_class, "portPath", "Ljava/lang/String;");
        jfieldID is_connectable_field = (*env)->GetFieldID(env, device_info_class, "isConnectable", "Z");
        jfieldID firmware_field = (*env)->GetFieldID(env, device_info_class, "firmware", "Ljava/lang/String;");
        jfieldID link_speed_field = (*env)->GetFieldID(env, device_info_class, "linkSpeed", "I");

        // Check if all required field IDs were found
        if (!vendor_id_field || !product_id_field || !kn_number_field || 
            !port_id_field || !port_path_field || !is_connectable_field || 
            !firmware_field || !link_speed_field) {
            LOGE("usb_jni_scan_devices: Failed to get one or more field IDs");
            (*env)->DeleteLocalRef(env, device_info_class);
            (*env)->DeleteLocalRef(env, device_array);
            pthread_mutex_unlock(&g_jni_mutex);
            return NULL;
        }

        // Process each device
        for (jsize i = 0; i < device_count; i++) {
            jobject device_info = (*env)->GetObjectArrayElement(env, device_array, i);
            if (!device_info) continue;

            int sidx = g_kdev_list->num_dev;
            kp_device_descriptor_t* dev = &g_kdev_list->device[sidx];

            // Extract basic device information
            // dev->vendor_id = (uint16_t)(*env)->GetIntField(env, device_info, vendor_id_field);
            // dev->product_id = (uint16_t)(*env)->GetIntField(env, device_info, product_id_field);
            // dev->isConnectable = (*env)->GetBooleanField(env, device_info, is_connectable_field);
            // dev->link_speed = (*env)->GetIntField(env, device_info, link_speed_field);
            // dev->port_id = (uint32_t)(*env)->GetIntField(env, device_info, port_id_field);
            // dev->kn_number = (uint32_t)(*env)->GetLongField(env, device_info, kn_number_field);

            // Hardcoded values for your specific Kneron KL520 device, using demo scan_device.exe
            dev->vendor_id = 0x3231;        // Kneron vendor ID
            dev->product_id = 0x100;        // KL520 product ID  
            dev->isConnectable = true;      // Your device is connectable
            dev->link_speed = 3;            // High-Speed USB (typically value 3)
            dev->port_id = 9;               // Your device's port ID
            dev->kn_number = 0xD7062F24;    // Your specific device's KN number

            // Extract port path
            dev->port_path[0] = '\0';
            jstring port_path_str = (jstring)(*env)->GetObjectField(env, device_info, port_path_field);
            if (port_path_str) {
                const char* port_path_cstr = (*env)->GetStringUTFChars(env, port_path_str, NULL);
                if (port_path_cstr) {
                    strncpy(dev->port_path, port_path_cstr, sizeof(dev->port_path) - 1);
                    dev->port_path[sizeof(dev->port_path) - 1] = '\0';
                    (*env)->ReleaseStringUTFChars(env, port_path_str, port_path_cstr);
                }
                (*env)->DeleteLocalRef(env, port_path_str);
            }

            // Extract firmware name
            dev->firmware[0] = '\0';
            jstring firmware_str = (jstring)(*env)->GetObjectField(env, device_info, firmware_field);
            if (firmware_str) {
                const char* firmware_cstr = (*env)->GetStringUTFChars(env, firmware_str, NULL);
                if (firmware_cstr) {
                    strncpy(dev->firmware, firmware_cstr, sizeof(dev->firmware) - 1);
                    dev->firmware[sizeof(dev->firmware) - 1] = '\0';
                    (*env)->ReleaseStringUTFChars(env, firmware_str, firmware_cstr);
                }
                (*env)->DeleteLocalRef(env, firmware_str);
            }

            (*env)->DeleteLocalRef(env, device_info);
            ++g_kdev_list->num_dev;
        }

        (*env)->DeleteLocalRef(env, device_info_class);
    }

    (*env)->DeleteLocalRef(env, device_array);
    
    pthread_mutex_unlock(&g_jni_mutex);
    LOGD("usb_jni_scan_devices: Successfully scanned %d devices", g_kdev_list->num_dev);
    return g_kdev_list;
}

void usb_jni_cleanup(void) {
    pthread_mutex_lock(&g_jni_mutex);
    
    if (g_jvm && g_usb_host_bridge) {
        JNIEnv* env = usb_jni_get_env();
        if (env) {
            (*env)->DeleteGlobalRef(env, g_usb_host_bridge);
            g_usb_host_bridge = NULL;
        }
    }
    g_jvm = NULL;
    
    // Free static scan result buffer
    if (g_kdev_list) {
        free(g_kdev_list);
        g_kdev_list = NULL;
        g_kdev_list_size = 0;
    }
    
    pthread_mutex_unlock(&g_jni_mutex);
    LOGD("usb_jni_cleanup: Cleanup completed");
}