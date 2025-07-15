/**
 * @file        kl520_ffi_interface.h
 * @brief       Header file for Kneron KL520 FFI interface
 * @version     1.0
 * @date        2025-07-01
 *
**/

#ifndef KL520_FFI_INTERFACE_H
#define KL520_FFI_INTERFACE_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Device connection and initialization
int scan_devices(int argc, char *argv[]);
int ffi_kp_connect_device();
int ffi_kp_load_firmware(const char *scpu_path, const char *ncpu_path);
int ffi_kp_load_model(const char *model_path);
int ffi_kp_configure_inference(bool enable_frame_drop);
int ffi_kp_disconnect_device();

// Image processing
int ffi_kp_send_image(uint8_t *buffer, int width, int height);
int ffi_kp_receive_result(float *out_result_buffer, int max_buffer_size);

// Utility
int ffi_kp_get_model_resolution(int *width, int *height);

#ifdef __cplusplus
}
#endif

#endif // KL520_FFI_INTERFACE_H
