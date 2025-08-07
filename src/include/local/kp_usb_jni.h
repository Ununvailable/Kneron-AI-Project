#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <jni.h>
#include <stdint.h>

// Opaque handle to abstract the underlying UsbDeviceConnection in Android
typedef struct usb_device_handle usb_device_handle_t;

/**
 * Initialize USB abstraction layer
 * (JNI registration, UsbManager binding, etc.)
 */
int usb_transport_initialize(JNIEnv* env, jobject usb_host_bridge);

/**
 * Finalize USB abstraction layer
 * (Release JNI refs, cleanup)
 */
int usb_transport_finalize(JNIEnv* env);

/**
 * Open a USB device by Vendor ID and Product ID
 * Returns a device handle on success, NULL on failure
 */
usb_device_handle_t* usb_transport_open(uint16_t vendor_id, uint16_t product_id);

/**
 * Close a previously opened USB device handle
 */
int usb_transport_close(usb_device_handle_t* handle);

/**
 * Perform a bulk transfer OUT (host to device)
 */
int usb_transport_bulk_out(usb_device_handle_t* handle, uint8_t endpoint, const void* data, int length, int* transferred, int timeout_ms);

/**
 * Perform a bulk transfer IN (device to host)
 */
int usb_transport_bulk_in(usb_device_handle_t* handle, uint8_t endpoint, void* data, int length, int* transferred, int timeout_ms);

/**
 * Perform a control transfer
 */
int usb_transport_control_transfer(usb_device_handle_t* handle, uint8_t request_type, uint8_t request,
                                   uint16_t value, uint16_t index, void* data, uint16_t length, int timeout_ms);

#ifdef __cplusplus
}
#endif
