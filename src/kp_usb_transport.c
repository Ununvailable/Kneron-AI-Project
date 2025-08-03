#include "kp_usb_transport.h"
#include <stdlib.h>
#include <string.h>
#include <android/log.h>

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

int usb_transport_initialize(JNIEnv* env, jobject usb_host_bridge) {
    if (!env || !usb_host_bridge) {
        LOGE("usb_transport_initialize: Invalid parameters");
        return -1;
    }

    // Store JavaVM reference
    if ((*env)->GetJavaVM(env, &g_jvm) != JNI_OK) {
        LOGE("usb_transport_initialize: Failed to get JavaVM");
        return -2;
    }

    // Store global reference to USB host bridge
    g_usb_host_bridge = (*env)->NewGlobalRef(env, usb_host_bridge);
    if (!g_usb_host_bridge) {
        LOGE("usb_transport_initialize: Failed to create global reference");
        return -3;
    }

    LOGD("usb_transport_initialize: Successfully initialized USB transport");
    return 0;
}

int usb_transport_finalize(JNIEnv* env) {
    if (!env) return -1;

    if (g_usb_host_bridge) {
        (*env)->DeleteGlobalRef(env, g_usb_host_bridge);
        g_usb_host_bridge = NULL;
    }

    g_jvm = NULL;
    LOGD("usb_transport_finalize: Finalized USB transport");
    return 0;
}

usb_device_handle_t* usb_transport_open(uint16_t vendor_id, uint16_t product_id) {
    if (!g_jvm) {
        LOGE("usb_transport_open: Transport not initialized");
        return NULL;
    }

    JNIEnv* env = NULL;
    if ((*g_jvm)->GetEnv(g_jvm, (void**)&env, JNI_VERSION_1_6) != JNI_OK) {
        LOGE("usb_transport_open: Failed to get JNI environment");
        return NULL;
    }

    // Allocate device handle
    usb_device_handle_t* handle = (usb_device_handle_t*)malloc(sizeof(usb_device_handle_t));
    if (!handle) {
        LOGE("usb_transport_open: Failed to allocate memory");
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
        LOGE("usb_transport_open: Failed to get UsbHostBridge class");
        free(handle);
        return NULL;
    }

    // Cache method IDs
    handle->bulk_out_method = (*env)->GetMethodID(env, handle->bridge_class,
        "bulkTransferOut", "(I[BII)I");
    if (!handle->bulk_out_method) {
        LOGE("usb_transport_open: Failed to get bulkTransferOut method");
        (*env)->DeleteLocalRef(env, handle->bridge_class);
        free(handle);
        return NULL;
    }

    handle->bulk_in_method = (*env)->GetMethodID(env, handle->bridge_class,
        "bulkTransferIn", "(I[BI[II)I");
    if (!handle->bulk_in_method) {
        LOGE("usb_transport_open: Failed to get bulkTransferIn method");
        (*env)->DeleteLocalRef(env, handle->bridge_class);
        free(handle);
        return NULL;
    }

    handle->control_method = (*env)->GetMethodID(env, handle->bridge_class,
        "controlTransfer", "(IIII[BII)I");
    if (!handle->control_method) {
        LOGE("usb_transport_open: Failed to get controlTransfer method");
        (*env)->DeleteLocalRef(env, handle->bridge_class);
        free(handle);
        return NULL;
    }

    // Set default endpoints (these would typically be determined during device enumeration)
    handle->endpoint_bulk_in = 0x81;   // IN endpoint 1
    handle->endpoint_bulk_out = 0x01;  // OUT endpoint 1

    LOGD("usb_transport_open: Successfully opened device VID:0x%04x PID:0x%04x", 
         vendor_id, product_id);
    return handle;
}

int usb_transport_close(usb_device_handle_t* handle) {
    if (!handle) {
        LOGE("usb_transport_close: Invalid handle");
        return -1;
    }

    if (handle->env && handle->bridge_class) {
        (*handle->env)->DeleteLocalRef(handle->env, handle->bridge_class);
    }

    free(handle);
    LOGD("usb_transport_close: Closed device handle");
    return 0;
}

int usb_transport_bulk_out(usb_device_handle_t* handle, uint8_t endpoint, const void* data, int length, int* transferred, int timeout_ms) {
    if (!handle || !data || length <= 0) {
        LOGE("usb_transport_bulk_out: Invalid parameters");
        return -1;
    }

    JNIEnv* env = handle->env;
    
    // Create byte array and copy data
    jbyteArray byte_array = (*env)->NewByteArray(env, length);
    if (!byte_array) {
        LOGE("usb_transport_bulk_out: Failed to create byte array");
        return -2;
    }

    (*env)->SetByteArrayRegion(env, byte_array, 0, length, (const jbyte*)data);
    if ((*env)->ExceptionCheck(env)) {
        LOGE("usb_transport_bulk_out: Failed to set byte array data");
        (*env)->ExceptionClear(env);
        (*env)->DeleteLocalRef(env, byte_array);
        return -3;
    }

    // Call bulkTransferOut method
    jint result = (*env)->CallIntMethod(env, handle->usb_host_bridge, 
                                       handle->bulk_out_method,
                                       (jint)endpoint, byte_array, 
                                       (jint)length, (jint)timeout_ms);

    (*env)->DeleteLocalRef(env, byte_array);

    if ((*env)->ExceptionCheck(env)) {
        LOGE("usb_transport_bulk_out: Exception during bulk transfer");
        (*env)->ExceptionDescribe(env);
        (*env)->ExceptionClear(env);
        return -4;
    }

    return (int)result;
}

int usb_transport_bulk_in(usb_device_handle_t* handle, uint8_t endpoint, void* data, int length, int* transferred, int timeout_ms) {
    if (!handle || !data || length <= 0 || !transferred) {
        LOGE("usb_transport_bulk_in: Invalid parameters");
        return -1;
    }

    JNIEnv* env = handle->env;
    
    // Create byte array for receiving data
    jbyteArray byte_array = (*env)->NewByteArray(env, length);
    if (!byte_array) {
        LOGE("usb_transport_bulk_in: Failed to create byte array");
        return -2;
    }

    // Create int array for transferred count
    jintArray transferred_array = (*env)->NewIntArray(env, 1);
    if (!transferred_array) {
        LOGE("usb_transport_bulk_in: Failed to create transferred array");
        (*env)->DeleteLocalRef(env, byte_array);
        return -3;
    }

    // Call bulkTransferIn method
    jint result = (*env)->CallIntMethod(env, handle->usb_host_bridge,
                                       handle->bulk_in_method,
                                       (jint)endpoint, byte_array,
                                       (jint)length, transferred_array,
                                       (jint)timeout_ms);

    if ((*env)->ExceptionCheck(env)) {
        LOGE("usb_transport_bulk_in: Exception during bulk transfer");
        (*env)->ExceptionDescribe(env);
        (*env)->ExceptionClear(env);
        (*env)->DeleteLocalRef(env, byte_array);
        (*env)->DeleteLocalRef(env, transferred_array);
        return -4;
    }

    // Get transferred count
    jint transferred_count = 0;
    (*env)->GetIntArrayRegion(env, transferred_array, 0, 1, &transferred_count);
    *transferred = (int)transferred_count;

    // Copy received data back to buffer
    if (transferred_count > 0) {
        (*env)->GetByteArrayRegion(env, byte_array, 0, transferred_count, (jbyte*)data);
        if ((*env)->ExceptionCheck(env)) {
            LOGE("usb_transport_bulk_in: Failed to get byte array data");
            (*env)->ExceptionClear(env);
            (*env)->DeleteLocalRef(env, byte_array);
            (*env)->DeleteLocalRef(env, transferred_array);
            return -5;
        }
    }

    (*env)->DeleteLocalRef(env, byte_array);
    (*env)->DeleteLocalRef(env, transferred_array);

    return (result >= 0) ? 0 : (int)result;
}

int usb_transport_control_transfer(usb_device_handle_t* handle, uint8_t request_type,
                                  uint8_t request, uint16_t value, uint16_t index,
                                  void* data, uint16_t length, int timeout_ms) {
    if (!handle) {
        LOGE("usb_transport_control_transfer: Invalid handle");
        return -1;
    }

    JNIEnv* env = handle->env;
    jbyteArray byte_array = NULL;

    // Create byte array if data is provided
    if (data && length > 0) {
        byte_array = (*env)->NewByteArray(env, length);
        if (!byte_array) {
            LOGE("usb_transport_control_transfer: Failed to create byte array");
            return -2;
        }

        // For OUT transfers, copy data to array
        if ((request_type & 0x80) == 0) {  // Direction: Host to Device
            (*env)->SetByteArrayRegion(env, byte_array, 0, length, (const jbyte*)data);
            if ((*env)->ExceptionCheck(env)) {
                LOGE("usb_transport_control_transfer: Failed to set byte array data");
                (*env)->ExceptionClear(env);
                (*env)->DeleteLocalRef(env, byte_array);
                return -3;
            }
        }
    }

    // Call controlTransfer method
    jint result = (*env)->CallIntMethod(env, handle->usb_host_bridge,
                                       handle->control_method,
                                       (jint)request_type, (jint)request,
                                       (jint)value, (jint)index,
                                       byte_array, (jint)length,
                                       (jint)timeout_ms);

    if ((*env)->ExceptionCheck(env)) {
        LOGE("usb_transport_control_transfer: Exception during control transfer");
        (*env)->ExceptionDescribe(env);
        (*env)->ExceptionClear(env);
        if (byte_array) (*env)->DeleteLocalRef(env, byte_array);
        return -4;
    }

    // For IN transfers, copy data back from array
    if (data && length > 0 && (request_type & 0x80) != 0 && result > 0) {
        (*env)->GetByteArrayRegion(env, byte_array, 0, result, (jbyte*)data);
        if ((*env)->ExceptionCheck(env)) {
            LOGE("usb_transport_control_transfer: Failed to get byte array data");
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