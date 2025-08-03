#include "kp_usb_jni_bridge.h"
#include <android/log.h>
#include <pthread.h>

#define LOG_TAG "KP_USB_JNI"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// Global reference to USB connection object and cached method IDs
static jobject g_usb_connection = NULL;
static JavaVM* g_jvm = NULL;
static jclass g_usb_connection_class = NULL;
static jmethodID g_bulk_transfer_method = NULL;
static jmethodID g_control_transfer_method = NULL;

static pthread_mutex_t g_jni_mutex = PTHREAD_MUTEX_INITIALIZER;

int usb_jni_register(JNIEnv* env, jobject usb_connection_obj) {
    if (!env || !usb_connection_obj) {
        LOGE("usb_jni_register: Invalid parameters");
        return -1;
    }

    pthread_mutex_lock(&g_jni_mutex);

    if (g_usb_connection) {
        (*env)->DeleteGlobalRef(env, g_usb_connection);
        g_usb_connection = NULL;
    }
    if (g_usb_connection_class) {
        (*env)->DeleteGlobalRef(env, g_usb_connection_class);
        g_usb_connection_class = NULL;
    }

    if ((*env)->GetJavaVM(env, &g_jvm) != JNI_OK) {
        LOGE("usb_jni_register: Failed to get JavaVM");
        pthread_mutex_unlock(&g_jni_mutex);
        return -2;
    }

    g_usb_connection = (*env)->NewGlobalRef(env, usb_connection_obj);
    if (!g_usb_connection) {
        LOGE("usb_jni_register: Failed to create global reference");
        pthread_mutex_unlock(&g_jni_mutex);
        return -3;
    }

    jclass local_class = (*env)->GetObjectClass(env, usb_connection_obj);
    if (!local_class) {
        LOGE("usb_jni_register: Failed to get UsbDeviceConnection class");
        usb_jni_cleanup(env);
        pthread_mutex_unlock(&g_jni_mutex);
        return -4;
    }

    g_usb_connection_class = (*env)->NewGlobalRef(env, local_class);
    (*env)->DeleteLocalRef(env, local_class);

    if (!g_usb_connection_class) {
        LOGE("usb_jni_register: Failed to create global class reference");
        usb_jni_cleanup(env);
        pthread_mutex_unlock(&g_jni_mutex);
        return -5;
    }

    g_bulk_transfer_method = (*env)->GetMethodID(env, g_usb_connection_class,
        "bulkTransfer", "(I[BIII)I");
    if (!g_bulk_transfer_method) {
        LOGE("usb_jni_register: Failed to get bulkTransfer method ID");
        usb_jni_cleanup(env);
        pthread_mutex_unlock(&g_jni_mutex);
        return -6;
    }

    g_control_transfer_method = (*env)->GetMethodID(env, g_usb_connection_class,
        "controlTransfer", "(IIII[BIII)I");
    if (!g_control_transfer_method) {
        LOGE("usb_jni_register: Failed to get controlTransfer method ID");
        usb_jni_cleanup(env);
        pthread_mutex_unlock(&g_jni_mutex);
        return -7;
    }

    LOGD("usb_jni_register: Successfully registered USB JNI bridge");
    pthread_mutex_unlock(&g_jni_mutex);
    return 0;
}

void usb_jni_cleanup(JNIEnv* env) {
    if (!env) return;

    pthread_mutex_lock(&g_jni_mutex);
    if (g_usb_connection) {
        (*env)->DeleteGlobalRef(env, g_usb_connection);
        g_usb_connection = NULL;
    }
    if (g_usb_connection_class) {
        (*env)->DeleteGlobalRef(env, g_usb_connection_class);
        g_usb_connection_class = NULL;
    }
    g_bulk_transfer_method = NULL;
    g_control_transfer_method = NULL;
    g_jvm = NULL;
    pthread_mutex_unlock(&g_jni_mutex);

    LOGD("usb_jni_cleanup: Cleaned up USB JNI bridge");
}

static int call_bulk_transfer(JNIEnv* env, jint endpoint, jbyteArray data, jint length, jint timeout_ms) {
    return (*env)->CallIntMethod(env, g_usb_connection, g_bulk_transfer_method,
                                 endpoint, data, 0, length, timeout_ms);
}

int usb_jni_bulk_out(JNIEnv* env, jobject unused, jint endpoint,
                     jbyteArray data, jint length, jint timeout_ms) {
    if (!env || !g_usb_connection || !g_bulk_transfer_method || !data) {
        LOGE("usb_jni_bulk_out: Invalid JNI state or parameters");
        return -1;
    }

    jint result = call_bulk_transfer(env, endpoint, data, length, timeout_ms);

    if ((*env)->ExceptionCheck(env)) {
        LOGE("usb_jni_bulk_out: Exception during bulk transfer");
        (*env)->ExceptionDescribe(env);
        (*env)->ExceptionClear(env);
        return -2;
    }
    return result;
}

int usb_jni_bulk_in(JNIEnv* env, jobject unused, jint endpoint,
                    jbyteArray buffer, jint length, jintArray transferred, jint timeout_ms) {
    if (!env || !g_usb_connection || !g_bulk_transfer_method || !buffer || !transferred) {
        LOGE("usb_jni_bulk_in: Invalid JNI state or parameters");
        return -1;
    }

    jint result = call_bulk_transfer(env, endpoint, buffer, length, timeout_ms);

    if ((*env)->ExceptionCheck(env)) {
        LOGE("usb_jni_bulk_in: Exception during bulk transfer");
        (*env)->ExceptionDescribe(env);
        (*env)->ExceptionClear(env);
        return -2;
    }

    jint bytes_transferred = (result >= 0) ? result : 0;
    (*env)->SetIntArrayRegion(env, transferred, 0, 1, &bytes_transferred);

    if ((*env)->ExceptionCheck(env)) {
        LOGE("usb_jni_bulk_in: Exception setting transferred");
        (*env)->ExceptionClear(env);
        return -3;
    }

    return (result >= 0) ? 0 : result;
}

int usb_jni_control_transfer(JNIEnv* env, jobject unused,
                             jint request_type, jint request, jint value, jint index,
                             jbyteArray buffer, jint length, jint timeout_ms) {
    if (!env || !g_usb_connection || !g_control_transfer_method) {
        LOGE("usb_jni_control_transfer: Invalid JNI state or parameters");
        return -1;
    }

    jint result = (*env)->CallIntMethod(env, g_usb_connection, g_control_transfer_method,
                                        request_type, request, value, index, buffer, 0, length, timeout_ms);

    if ((*env)->ExceptionCheck(env)) {
        LOGE("usb_jni_control_transfer: Exception during control transfer");
        (*env)->ExceptionDescribe(env);
        (*env)->ExceptionClear(env);
        return -2;
    }

    return result;
}
