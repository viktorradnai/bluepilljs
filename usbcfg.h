#ifndef _USBCFG_H_
#define _USBCFG_H_

#define usb_lld_connect_bus(usbp)
#define usb_lld_disconnect_bus(usbp)
extern const USBConfig usbcfg;
extern SerialUSBConfig serusbcfg;
extern SerialUSBDriver SDU1;

#endif  /* _USBCFG_H_ */
