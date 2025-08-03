#pragma once

#include <jni.h>
#include <stdint.h>  // for standard int types

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initializes JNI USB bridge by registering a global reference
 * to the Java UsbDeviceConnection instance.
 * Should be called from JNI_OnLoad().
 *
 * @param env                  JNI environment pointer
 * @param usb_connection_obj   Java UsbDeviceConnection object (must be non-null)
 * @return 0 on success, < 0 on failure
 */
int usb_jni_register(JNIEnv* env, jobject usb_connection_obj);

/**
 * Releases any cached global references.
 * Should be called from JNI_OnUnload() or when USB connection is closed.
 */
void usb_jni_cleanup(JNIEnv* env);

/**
 * Performs a USB bulk OUT transfer (host-to-device).
 *
 * @param env              JNI environment pointer
 * @param usb_connection   Java UsbDeviceConnection object
 * @param endpoint         Endpoint address (must be OUT type)
 * @param data             Byte array to send
 * @param length           Number of bytes to send
 * @param timeout_ms       Timeout in milliseconds
 * @return Number of bytes written, or negative on error
 */
int usb_jni_bulk_out(JNIEnv* env, jobject usb_connection, jint endpoint,
                     jbyteArray data, jint length, jint timeout_ms);

/**
 * Performs a USB bulk IN transfer (device-to-host).
 *
 * @param env              JNI environment pointer
 * @param usb_connection   Java UsbDeviceConnection object
 * @param endpoint         Endpoint address (must be IN type)
 * @param buffer           Byte array to receive data
 * @param length           Number of bytes to receive
 * @param transferred      Integer array of size 1 to store actual bytes read
 * @param timeout_ms       Timeout in milliseconds
 * @return 0 on success, negative on error
 */
int usb_jni_bulk_in(JNIEnv* env, jobject usb_connection, jint endpoint,
                    jbyteArray buffer, jint length, jintArray transferred, jint timeout_ms);

/**
 * Performs a USB control transfer.
 *
 * @param env              JNI environment pointer
 * @param usb_connection   Java UsbDeviceConnection object
 * @param request_type     Request type (bmRequestType)
 * @param request          Request (bRequest)
 * @param value            wValue field
 * @param index            wIndex field
 * @param buffer           Data buffer (in/out depending on request direction)
 * @param length           Length of data buffer
 * @param timeout_ms       Timeout in milliseconds
 * @return Number of bytes transferred, or negative on error
 */
int usb_jni_control_transfer(JNIEnv* env, jobject usb_connection,
                             jint request_type, jint request, jint value, jint index,
                             jbyteArray buffer, jint length, jint timeout_ms);

#ifdef __cplusplus
}
#endif
