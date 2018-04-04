#ifndef _USBCFG_H_
#define _USBCFG_H_

#define usb_lld_connect_bus(usbp)
#define usb_lld_disconnect_bus(usbp)
extern SerialUSBDriver SDU1;
extern uint8_t usbInitState;

/*
 * Endpoints to be used for USBD1.
 */
//#define HID_CONTROL_EP_ADDRESS    0   /* Implicit */
#define HID_IN_EP_ADDRESS       3   /* Interrupt. Mandatory */
#define HID_OUT_EP_ADDRESS      4   /* Interrupt. Optional */
#define USBD1_DATA_REQUEST_EP           1
#define USBD1_DATA_AVAILABLE_EP         1
#define USBD1_INTERRUPT_REQUEST_EP      2

#endif  /* _USBCFG_H_ */
