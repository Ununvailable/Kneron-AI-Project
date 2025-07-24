#pragma once

#include <jni.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Should be called from JNI_OnLoad to register Java-side USBManager object
 */
int usb_jni_register(JNIEnv* env, jobject usb_manager_instance);

/**
 * JNI forwarding for bulk OUT transfer
 */
int usb_jni_bulk_out(JNIEnv* env, jobject usb_manager, jint endpoint, jbyteArray data, jint length, jint timeout_ms);

/**
 * JNI forwarding for bulk IN transfer
 */
int usb_jni_bulk_in(JNIEnv* env, jobject usb_manager, jint endpoint, jbyteArray buffer, jint length, jintArray transferred, jint timeout_ms);

/**
 * JNI forwarding for control transfer
 */
int usb_jni_control_transfer(JNIEnv* env, jobject usb_manager, jint request_type, jint request,
                              jint value, jint index, jbyteArray buffer, jint length, jint timeout_ms);

#ifdef __cplusplus
}
#endif
