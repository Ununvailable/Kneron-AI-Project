#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "kp_usb_jni_bridge.h"

// Opaque handle to abstract the underlying UsbDeviceConnection in Android
typedef struct usb_device_handle usb_device_handle_t;

// /** \ingroup libusb_dev
//  * Speed codes. Indicates the speed at which the device is operating. Also present in kp_struct
//  */
// enum usb_speed {
// 	/** The OS doesn't report or know the device speed. */
// 	KP_USB_SPEED_UNKNOWN = 0,

// 	/** The device is operating at low speed (1.5MBit/s). */
// 	KP_USB_SPEED_LOW = 1,

// 	/** The device is operating at full speed (12MBit/s). */
// 	KP_USB_SPEED_FULL = 2,

// 	/** The device is operating at high speed (480MBit/s). */
// 	KP_USB_SPEED_HIGH = 3,

// 	/** The device is operating at super speed (5000MBit/s). */
// 	KP_USB_SPEED_SUPER = 4,

// 	/** The device is operating at super speed plus (10000MBit/s). */
// 	KP_USB_SPEED_SUPER_PLUS = 5,

// 	/** The device is operating at super speed plus x2 (20000MBit/s). */
// 	KP_USB_SPEED_SUPER_PLUS_X2 = 6,
// };


/** \ingroup libusb_desc
 * Endpoint direction. Values for bit 7 of the
 * \ref libusb_endpoint_descriptor::bEndpointAddress "endpoint address" scheme.
 */
enum kp_usb_endpoint_direction {
	/** Out: host-to-device */
	KP_USB_ENDPOINT_OUT = 0x00,

	/** In: device-to-host */
	KP_USB_ENDPOINT_IN = 0x80
};

/** \ingroup libusb_misc
 * Request type bits of the
 * \ref libusb_control_setup::bmRequestType "bmRequestType" field in control
 * transfers. */
enum kp_usb_request_type {
	/** Standard */
	KP_USB_REQUEST_TYPE_STANDARD = (0x00 << 5),

	/** Class */
	KP_USB_REQUEST_TYPE_CLASS = (0x01 << 5),

	/** Vendor */
	KP_USB_REQUEST_TYPE_VENDOR = (0x02 << 5),

	/** Reserved */
	KP_USB_REQUEST_TYPE_RESERVED = (0x03 << 5)
};

/** \ingroup libusb_misc
 * Recipient bits of the
 * \ref libusb_control_setup::bmRequestType "bmRequestType" field in control
 * transfers. Values 4 through 31 are reserved. */
enum kp_usb_request_recipient {
	/** Device */
	KP_USB_RECIPIENT_DEVICE = 0x00,

	/** Interface */
	KP_USB_RECIPIENT_INTERFACE = 0x01,

	/** Endpoint */
	KP_USB_RECIPIENT_ENDPOINT = 0x02,

	/** Other */
	KP_USB_RECIPIENT_OTHER = 0x03
};

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
