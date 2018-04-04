/*
    ChibiOS - Copyright (C) 2006..2015 Giovanni Di Sirio
    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at
        http://www.apache.org/licenses/LICENSE-2.0
    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/
typedef int bool_t;

#ifndef _USB_HID_H_
#define _USB_HID_H_

/* HID Class Specific Requests */
#define HID_GET_REPORT_REQUEST      0x01
#define HID_GET_IDLE_REQUEST        0x02
#define HID_GET_PROTOCOL_REQUEST    0x03
#define HID_SET_REPORT_REQUEST      0x09
#define HID_SET_IDLE_REQUEST        0x0A
#define HID_SET_PROTOCOL_REQUEST    0x0B
/* --------------------------- */

#define USB_DESCRIPTOR_HID      0x21

#define HID_USAGE_PAGE(up)      (uint8_t)(0x05 & 255),(uint8_t)(((up)) & 255)
#define HID_USAGE(u)            (uint8_t)(0x09 & 255),(uint8_t)(((u)) & 255)
#define HID_COLLECTION(c)       (uint8_t)(0xA1 & 255),(uint8_t)(((c)) & 255)
#define HID_END_COLLECTION      0xC0

#define HID_USAGE_MINIMUM(x)        (uint8_t)(0x19 & 255),(uint8_t)(((x)) & 255)
#define HID_USAGE_MAXIMUM(x)        (uint8_t)(0x29 & 255),(uint8_t)(((x)) & 255)
#define HID_LOGICAL_MINIMUM(x)      (uint8_t)(0x15 & 255),(uint8_t)(((x)) & 255)
#define HID_LOGICAL_MAXIMUM(x)      (uint8_t)(0x25 & 255),(uint8_t)(((x)) & 255)
#define HID_REPORT_COUNT(x)         (uint8_t)(0x95 & 255),(uint8_t)(((x)) & 255)
#define HID_REPORT_SIZE(x)          (uint8_t)(0x75 & 255),(uint8_t)(((x)) & 255)
#define HID_INPUT(x)            (uint8_t)(0x81 & 255),(uint8_t)(((x)) & 255)

#define HID_COLLECTION_PHYSICAL     0x00
#define HID_COLLECTION_APPLICATION  0x01
#define HID_COLLECTION_LOGICAL      0x02
#define HID_COLLECTION_REPORT       0x03
#define HID_COLLECTION_NAMED_ARRAY  0x04
#define HID_COLLECTION_USAGE_SWITCH 0x05
#define HID_COLLECTION_USAGE_MODIFIER   0x06

#define HID_USAGE_PAGE_GENERIC_DESKTOP  0x01
#define HID_USAGE_PAGE_SIMULATION   0x02
#define HID_USAGE_PAGE_VR       0x03
#define HID_USAGE_PAGE_SPORT        0x04
#define HID_USAGE_PAGE_GAME     0x05
#define HID_USAGE_PAGE_GENERIC_DEVICE   0x06
#define HID_USAGE_PAGE_KEYBOARD_KEYPAD  0x07
#define HID_USAGE_PAGE_LEDS     0x08
#define HID_USAGE_PAGE_BUTTON       0x09
#define HID_USAGE_PAGE_ORDINAL      0x0A
#define HID_USAGE_PAGE_TELEPHONY    0x0B
#define HID_USAGE_PAGE_CONSUMER     0x0C
#define HID_USAGE_PAGE_DIGITIZER    0x0D
#define HID_USAGE_PAGE_PID      0x0F
#define HID_USAGE_PAGE_UNICODE      0x10
#define HID_USAGE_ALPHANUMERIC_DISPLAY  0x14
#define HID_USAGE_MEDICAL_INSTRUMENTS   0x40
#define HID_USAGE_MONITOR_PAGE1     0x80
#define HID_USAGE_MONITOR_PAGE2     0x81
#define HID_USAGE_MONITOR_PAGE3     0x82
#define HID_USAGE_MONITOR_PAGE4     0x83
#define HID_USAGE_POWER_PAGE1       0x84
#define HID_USAGE_POWER_PAGE2       0x85
#define HID_USAGE_POWER_PAGE3       0x86
#define HID_USAGE_POWER_PAGE4       0x87
#define HID_USAGE_BAR_CODE_SCANNER_PAGE 0x8C
#define HID_USAGE_SCALE_PAGE        0x8D
#define HID_USAGE_MSR_PAGE      0x8E
#define HID_USAGE_CAMERA_PAGE       0x90
#define HID_USAGE_ARCADE_PAGE       0x91

#define HID_USAGE_POINTER       0x01
#define HID_USAGE_MOUSE         0x02
#define HID_USAGE_JOYSTICK      0x04
#define HID_USAGE_GAMEPAD       0x05
#define HID_USAGE_KEYBOARD      0x06
#define HID_USAGE_KEYPAD        0x07
#define HID_USAGE_MULTIAXIS_CONTROLLER  0x08

#define HID_USAGE_BUTTON1         0x01
#define HID_USAGE_BUTTON8         0x08

#define HID_USAGE_X         0x30
#define HID_USAGE_Y         0x31
#define HID_USAGE_Z         0x32
#define HID_USAGE_RX            0x33
#define HID_USAGE_RY            0x34
#define HID_USAGE_RZ            0x35
#define HID_USAGE_VX            0x40
#define HID_USAGE_VY            0x41
#define HID_USAGE_VZ            0x42
#define HID_USAGE_VBRX          0x43
#define HID_USAGE_VBRY          0x44
#define HID_USAGE_VBRZ          0x45
#define HID_USAGE_VNO           0x46

#define HID_INPUT_DATA_VAR_ABS  0x02
#define HID_INPUT_CNST_VAR_ABS  0x03
#define HID_INPUT_DATA_VAR_REL  0x06


/**
 * @brief   Helper macro for index values into descriptor strings.
 */
#define USB_DESC_INDEX(i) ((uint8_t)(i))

/**
 * @brief   Helper macro for byte values into descriptor strings.
 */
#define USB_DESC_BYTE(b) ((uint8_t)(b))


#define USB_DESC_HID(bcdHID, bCountryCode, bNumDescriptors, bDescriptorType, wDescriptorLength) \
  USB_DESC_BYTE(9),\
  USB_DESC_BYTE(USB_DESCRIPTOR_HID),\
  USB_DESC_BCD(bcdHID),\
  USB_DESC_BYTE(bCountryCode),\
  USB_DESC_BYTE(bNumDescriptors),\
  USB_DESC_BYTE(bDescriptorType),\
  USB_DESC_WORD(wDescriptorLength)


typedef enum {
    HID_STATE_BUSY = 0,
    HID_STATE_FREE,
    HID_STATE_STALLED
} hid_usb_state_t;

typedef struct {
    int8_t x;
    uint8_t button;
} hid_data;

extern hid_data hid_in_data;
extern hid_data hid_out_data;
extern hid_usb_state_t hid_state;


#ifdef __cplusplus
extern "C" {
#endif
extern void hid_receive(USBDriver *usbp);
extern void hid_transmit(USBDriver *usbp) ;
extern bool_t hidRequestsHook(USBDriver *usbp);
extern void hidDataTransmitted(USBDriver *usbp, usbep_t ep);
extern  void hidDataReceived(USBDriver *usbp, usbep_t ep);
#ifdef __cplusplus
}
#endif

/**
 * @brief   Fetches a 16 bits word value from an USB message.
 *
 * @param[in] p         pointer to the 16 bits word
 *
 * @notapi
 */
#define usbFetchWord(p) ((uint16_t)*(p) | ((uint16_t)*((p) + 1) << 8))


#endif /* _USB_HID_H_ */


