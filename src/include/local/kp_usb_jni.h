#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <jni.h>
#include <stdint.h>
#include <stdbool.h>
#include "kp_struct.h"  // For kp_device_descriptor_t

// Opaque handle to abstract the underlying UsbDeviceConnection in Android
typedef struct usb_device_handle usb_device_handle_t;

/**
 * @brief Initialize USB abstraction layer.
 *        Must be called before other functions.
 *        Registers JNI methods, binds UsbManager, and prepares internal state.
 *
 * @param env JNI environment pointer for the current thread.
 * @param usb_host_bridge Java object providing USB host functionality (e.g., UsbHostBridge instance).
 * @return 0 on success, negative error code on failure.
 */
int usb_jni_initialize(JNIEnv* env, jobject usb_host_bridge);

/**
 * @brief Finalize USB abstraction layer.
 *        Releases JNI references and cleans up internal state.
 *
 * @param env JNI environment pointer for the current thread.
 * @return 0 on success, negative error code on failure.
 */
int usb_jni_finalize(JNIEnv* env);

/**
 * @brief Open a USB device identified by Vendor ID and Product ID.
 *
 * @param vendor_id Vendor ID of the USB device.
 * @param product_id Product ID of the USB device.
 * @return Pointer to a device handle on success; NULL on failure.
 *
 * @note If multiple devices share the same IDs, the first matched device is opened.
 *       Caller must call usb_jni_close() to release the handle.
 */
usb_device_handle_t* usb_jni_open(uint16_t vendor_id, uint16_t product_id);

/**
 * @brief Close a previously opened USB device handle.
 *
 * @param handle Device handle returned by usb_jni_open().
 * @return 0 on success, negative error code on failure.
 */
int usb_jni_close(usb_device_handle_t* handle);

/**
 * @brief Perform a bulk transfer OUT (host to device).
 *
 * @param handle Device handle.
 * @param endpoint Endpoint address (must have OUT direction bit set).
 * @param data Pointer to data buffer to send.
 * @param length Number of bytes to send.
 * @param transferred Pointer to int to receive number of bytes actually transferred.
 * @param timeout_ms Timeout in milliseconds.
 * @return 0 on success, negative error code on failure.
 */
int usb_jni_bulk_out(usb_device_handle_t* handle,
                     uint8_t endpoint,
                     const void* data,
                     int length,
                     int* transferred,
                     int timeout_ms);

/**
 * @brief Perform a bulk transfer IN (device to host).
 *
 * @param handle Device handle.
 * @param endpoint Endpoint address (must have IN direction bit set).
 * @param data Pointer to data buffer to receive data.
 * @param length Maximum number of bytes to receive.
 * @param transferred Pointer to int to receive number of bytes actually transferred.
 * @param timeout_ms Timeout in milliseconds.
 * @return 0 on success, negative error code on failure.
 */
int usb_jni_bulk_in(usb_device_handle_t* handle,
                    uint8_t endpoint,
                    void* data,
                    int length,
                    int* transferred,
                    int timeout_ms);

/**
 * @brief Perform a control transfer.
 *
 * @param handle Device handle.
 * @param request_type bmRequestType field of the setup packet.
 * @param request bRequest field of the setup packet.
 * @param value wValue field of the setup packet.
 * @param index wIndex field of the setup packet.
 * @param data Pointer to data buffer for data stage.
 * @param length Length of data buffer.
 * @param timeout_ms Timeout in milliseconds.
 * @return Number of bytes transferred on success (>=0), negative error code on failure.
 */
int usb_jni_control_transfer(usb_device_handle_t* handle,
                             uint8_t request_type,
                             uint8_t request,
                             uint16_t value,
                             uint16_t index,
                             void* data,
                             uint16_t length,
                             int timeout_ms);

/**
 * @brief Perform an interrupt transfer IN (device to host).
 *
 * @param handle Device handle.
 * @param endpoint Endpoint address (must have IN direction bit set).
 * @param data Pointer to data buffer to receive data.
 * @param length Maximum number of bytes to receive.
 * @param transferred Pointer to int to receive number of bytes actually transferred.
 * @param timeout_ms Timeout in milliseconds.
 * @return 0 on success, negative error code on failure.
 *
 * @note Android's USB API does not fully support interrupt transfers;
 *       behavior may vary by device.
 */
int usb_jni_interrupt_transfer_in(usb_device_handle_t* handle,
                                  uint8_t endpoint,
                                  void* data,
                                  int length,
                                  int* transferred,
                                  unsigned int timeout_ms);

/**
 * @brief Fill a kp_device_descriptor_t structure by querying Android USB device info via JNI.
 *
 * @param env JNI environment pointer for the current thread.
 * @param usb_host_bridge Java object providing USB host functionality.
 * @param vendor_id Vendor ID of the target device.
 * @param product_id Product ID of the target device.
 * @param desc Pointer to kp_device_descriptor_t to be populated.
 * @return 0 on success, negative error code on failure.
 *
 * @note If multiple devices match the vendor and product IDs, the first matching device
 *       is used. The function fills fields such as port_id, link_speed, kn_number,
 *       isConnectable, port_path, and firmware.
 */
int usb_jni_fill_device_descriptor(JNIEnv *env,
                                   jobject usb_host_bridge,
                                   uint16_t vendor_id,
                                   uint16_t product_id,
                                   kp_device_descriptor_t *desc);

#ifdef __cplusplus
}
#endif
