/*
    USB-HID Gamepad for ChibiOS/RT
    Copyright (C) 2014, +inf Wenzheng Xu.

    EMAIL: wx330@nyu.edu

    This piece of code is FREE SOFTWARE and is released
    under the Apache License, Version 2.0 (the "License");
*/

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


#include "ch.h"
#include "hal.h"
#include "usb_hid.h"
#define USB_DESCRIPTOR_REPORT 0x22

extern SerialUSBDriver SDU1;
/*===========================================================================*/
/* USB related stuff.                                                        */
/*===========================================================================*/

/* For a better understanding look at http://www.beyondlogic.org/usbnutshell/usb1.shtml */
/* Reference: [DCDHID] USB - Device Class Definition for Human Interface Devices (HID)  */
/*
 * USB Device Descriptor.
 */

static const uint8_t cdc_device_descriptor_data[] = {
  USB_DESC_DEVICE       (0x0110,        /* bcdUSB (1.1).                                                                */
                         0x00,          /* bDeviceClass (in interface).                                                 */
                         0x00,          /* bDeviceSubClass.                                                             */
                         0x00,          /* bDeviceProtocol.                                                             */
                         0x40,          /* bMaxPacketSize. Maximum Packet Size for Zero Endpoint (8,16,32,64)           */
                         0x0483,        /* idVendor: STMicroelectronics).                                               */
                         0x5710,        /* idProduct: Joystick in FS Mode                                               */
                         0x0100,        /* bcdDevice. Device Version Number assigned by the developer                   */
                         0x01,             /* iManufacturer.                                                            */
                         0x02,             /* iProduct.                                                                 */
                         0x03,             /* iSerialNumber.                                                            */
                         0x01)             /* bNumConfigurations. The system has only one configuration                 */
};

/*
 * Device Descriptor wrapper.
 */

static const USBDescriptor cdc_device_descriptor = {
  sizeof cdc_device_descriptor_data,
  cdc_device_descriptor_data
};


/*
 * USB Configuration Descriptor.
 */
static const uint8_t hid_generic_joystick_reporter_data[] ={
HID_USAGE_PAGE  (HID_USAGE_PAGE_GENERIC_DESKTOP),
HID_USAGE               (HID_USAGE_JOYSTICK),
HID_COLLECTION          (HID_COLLECTION_APPLICATION),
        HID_COLLECTION          (HID_COLLECTION_PHYSICAL),
            HID_USAGE_PAGE      (HID_USAGE_PAGE_GENERIC_DESKTOP),
            HID_USAGE           (HID_USAGE_X),
                // HID_USAGE               (HID_USAGE_Y),
                HID_LOGICAL_MINIMUM     (-127),
                HID_LOGICAL_MAXIMUM     (127),
                HID_REPORT_SIZE         (8),
                HID_REPORT_COUNT        (1),
                HID_INPUT       (HID_INPUT_DATA_VAR_ABS),
        HID_USAGE_PAGE(HID_USAGE_PAGE_BUTTON),
            HID_USAGE_MINIMUM(HID_USAGE_BUTTON1),
            HID_USAGE_MAXIMUM(HID_USAGE_BUTTON8),
            HID_LOGICAL_MINIMUM     (0),
            HID_LOGICAL_MAXIMUM     (1),
            HID_REPORT_SIZE             (1),
            HID_REPORT_COUNT    (8),
            HID_INPUT   (HID_INPUT_DATA_VAR_ABS),
        HID_END_COLLECTION ,
HID_END_COLLECTION,
};
static const USBDescriptor hid_generic_joystick_reporter = {
  sizeof hid_generic_joystick_reporter_data,
  hid_generic_joystick_reporter_data
};

static const uint8_t cdc_configuration_descriptor_data[] = {
  /* Configuration Descriptor.*/
  USB_DESC_CONFIGURATION(0x63,          /* wTotalLength.                    */
                         0x03,          /* bNumInterfaces.                  */
                         0x01,          /* bConfigurationValue.             */
                         0x00,          /* iConfiguration.                  */
                         0xC0,          /* bmAttributes (self powered).     */
                         0x50),         /* bMaxPower (100mA).               */
  /* Interface 1 Descriptor.*/
  USB_DESC_INTERFACE    (0x00,          /* bInterfaceNumber.                */
                         0x00,          /* bAlternateSetting.               */
                         0x01,          /* bNumEndpoints.                   */
                         0x02,          /* bInterfaceClass (Communications
                                           Interface Class, CDC section
                                           4.2).                            */
                         0x02,          /* bInterfaceSubClass (Abstract
                                         Control Model, CDC section 4.3).   */
                         0x01,          /* bInterfaceProtocol (AT commands,
                                           CDC section 4.4).                */
                         0),            /* iInterface.                      */
  /* Header Functional Descriptor (CDC section 5.2.3).*/
  USB_DESC_BYTE         (5),            /* bLength.                         */
  USB_DESC_BYTE         (0x24),         /* bDescriptorType (CS_INTERFACE).  */
  USB_DESC_BYTE         (0x00),         /* bDescriptorSubtype (Header
                                           Functional Descriptor.           */
  USB_DESC_BCD          (0x0110),       /* bcdCDC.                          */
  /* Call Management Functional Descriptor. */
  USB_DESC_BYTE         (5),            /* bFunctionLength.                 */
  USB_DESC_BYTE         (0x24),         /* bDescriptorType (CS_INTERFACE).  */
  USB_DESC_BYTE         (0x01),         /* bDescriptorSubtype (Call Management
                                           Functional Descriptor).          */
  USB_DESC_BYTE         (0x00),         /* bmCapabilities (D0+D1).          */
  USB_DESC_BYTE         (0x01),         /* bDataInterface.                  */
  /* ACM Functional Descriptor.*/
  USB_DESC_BYTE         (4),            /* bFunctionLength.                 */
  USB_DESC_BYTE         (0x24),         /* bDescriptorType (CS_INTERFACE).  */
  USB_DESC_BYTE         (0x02),         /* bDescriptorSubtype (Abstract
                                           Control Management Descriptor).  */
  USB_DESC_BYTE         (0x02),         /* bmCapabilities.                  */
  /* Union Functional Descriptor.*/
  USB_DESC_BYTE         (5),            /* bFunctionLength.                 */
  USB_DESC_BYTE         (0x24),         /* bDescriptorType (CS_INTERFACE).  */
  USB_DESC_BYTE         (0x06),         /* bDescriptorSubtype (Union
                                           Functional Descriptor).          */
  USB_DESC_BYTE         (0x00),         /* bMasterInterface (Communication
                                           Class Interface).                */
  USB_DESC_BYTE         (0x01),         /* bSlaveInterface0 (Data Class
                                           Interface).                      */
  /* Endpoint 2 Descriptor.*/
  USB_DESC_ENDPOINT     (USBD1_INTERRUPT_REQUEST_EP|0x80,
                         0x03,          /* bmAttributes (Interrupt).        */
                         0x0008,        /* wMaxPacketSize.                  */
                         0xFF),         /* bInterval.                       */
  /* Interface 2 Descriptor.*/
  USB_DESC_INTERFACE    (0x01,          /* bInterfaceNumber.                */
                         0x00,          /* bAlternateSetting.               */
                         0x02,          /* bNumEndpoints.                   */
                         0x0A,          /* bInterfaceClass (Data Class
                                           Interface, CDC section 4.5).     */
                         0x00,          /* bInterfaceSubClass (CDC section
                                           4.6).                            */
                         0x00,          /* bInterfaceProtocol (CDC section
                                           4.7).                            */
                         0x00),         /* iInterface.                      */
  /* Endpoint 3 Descriptor.*/
  USB_DESC_ENDPOINT     (USBD1_DATA_AVAILABLE_EP,       /* bEndpointAddress.*/
                         0x02,          /* bmAttributes (Bulk).             */
                         0x0040,        /* wMaxPacketSize.                  */
                         0x00),         /* bInterval.                       */
  /* Endpoint 1 Descriptor.*/
  USB_DESC_ENDPOINT     (USBD1_DATA_REQUEST_EP|0x80,    /* bEndpointAddress.*/
                         0x02,          /* bmAttributes (Bulk).             */
                         0x0040,        /* wMaxPacketSize.                  */
                         0x00),         /* bInterval.                       */


  /* Interface 3 (Joystick) Descriptor.*/
  USB_DESC_INTERFACE    (0x02,          /* bInterfaceNumber.                */
                         0x00,          /* bAlternateSetting.               */
                         0x02,          /* bNumEndpoints.                   */
                         0x03,          /* bInterfaceClass (Human Device Interface).   */
                         0x00,          /* bInterfaceSubClass  (DCDHID page 8)         */
                         0x00,          /* bInterfaceProtocol  (DCDHID page 9)         */
                         0x00),         /* iInterface.                                 */

  USB_DESC_HID              (0x0111,    /* bcdHID               */
                                     0x00,              /* bCountryCode         */
                                     0x01,              /* bNumDescriptor       */
                                     0x22,              /* bDescriptorType      */
                                     sizeof(hid_generic_joystick_reporter_data)),               /* Report Descriptor Lenght. Compute it! */
  /* Endpoint 1 Descriptor INTERRUPT IN  */
  USB_DESC_ENDPOINT     (HID_IN_EP_ADDRESS|0x80,          /* bEndpointAddress.*/
                         0x03,          /* bmAttributes (Interrupt).                                          */
                         0x04,          /* wMaxPacketSize.     0x40 = 64 BYTES                               */
                         0x04),         /* bInterval (Polling every 50ms)                                     */
  /* Endpoint 1 Descriptor INTERRUPT OUT */
  USB_DESC_ENDPOINT     (HID_OUT_EP_ADDRESS,          /* bEndpointAddress.*/
                         0x03,          /* bmAttributes (Interrupt).             */
                         0x04,          /* wMaxPacketSize.     0x40 = 64 BYTES  */
                         0x08),         /* bInterval (Polling every 50ms)        */
                          /* HID Report Descriptor */
                          /* Specific Class HID Descriptor */
};

/*
 * Configuration Descriptor wrapper.
 */
static const USBDescriptor cdc_configuration_descriptor = {
  sizeof cdc_configuration_descriptor_data,
  cdc_configuration_descriptor_data
};


/*
 * U.S. English language identifier.
 */
static const uint8_t hid_string0[] = {
  USB_DESC_BYTE(4),                     /* bLength.                         */
  USB_DESC_BYTE(USB_DESCRIPTOR_STRING), /* bDescriptorType.                 */
  USB_DESC_WORD(0x0409)                 /* wLANGID (U.S. English).          */
};

/*
 * Vendor string.
 */
static const uint8_t hid_string1[] = {
  USB_DESC_BYTE(38),                    /* bLength.                         */
  USB_DESC_BYTE(USB_DESCRIPTOR_STRING), /* bDescriptorType.                 */
  'S', 0, 'T', 0, 'M', 0, 'i', 0, 'c', 0, 'r', 0, 'o', 0, 'e', 0,
  'l', 0, 'e', 0, 'c', 0, 't', 0, 'r', 0, 'o', 0, 'n', 0, 'i', 0,
  'c', 0, 's', 0
};

/*
 * Device Description string.
 */
static const uint8_t hid_string2[] = {
  USB_DESC_BYTE(38),                    /* bLength.                         */
  USB_DESC_BYTE(USB_DESCRIPTOR_STRING), /* bDescriptorType.                 */
  'C', 0, 'h', 0, 'i', 0, 'b', 0, 'i', 0, 'O', 0, 'S', 0, '/', 0,
  'R', 0, 'T', 0, ' ', 0, 'U', 0, 'S', 0, 'B', 0, '-', 0, 'H', 0,
  'I', 0, 'D', 0
};

/*
 * Serial Number string.
 */
static const uint8_t hid_string3[] = {
  USB_DESC_BYTE(8),                     /* bLength.                         */
  USB_DESC_BYTE(USB_DESCRIPTOR_STRING), /* bDescriptorType.                 */
  '0' + CH_KERNEL_MAJOR, 0,
  '0' + CH_KERNEL_MINOR, 0,
  '0' + CH_KERNEL_PATCH, 0
};

/*
 * Interface string.
 */


static const uint8_t hid_string4[] = {
  USB_DESC_BYTE(16),                    /* bLength.                             */
  USB_DESC_BYTE(USB_DESCRIPTOR_STRING), /* bDescriptorType.                     */
  'U', 0, 'S', 0, 'B', 0, ' ', 0, 'H', 0, 'I', 0, 'D', 0
};

/*
 * Strings wrappers array.
 */
static const USBDescriptor hid_strings[] = {
  {sizeof hid_string0, hid_string0},
  {sizeof hid_string1, hid_string1},
  {sizeof hid_string2, hid_string2},
  {sizeof hid_string3, hid_string3},
  {sizeof hid_string4, hid_string4}
};

static const USBDescriptor *get_descriptor(USBDriver *usbp, uint8_t dtype, uint8_t dindex, uint16_t lang) {

  (void)usbp;
  (void)lang;
  switch (dtype) {
  case USB_DESCRIPTOR_DEVICE:
    return &cdc_device_descriptor;
  case USB_DESCRIPTOR_CONFIGURATION:
    return &cdc_configuration_descriptor;
  case USB_DESCRIPTOR_REPORT:
    //palSetPadMode(GPIOD, 12, PAL_MODE_OUTPUT_PUSHPULL);
    //palSetPad(GPIOD, 12);
    return &hid_generic_joystick_reporter;
  case USB_DESCRIPTOR_STRING:
    if (dindex < 5)
      return &hid_strings[dindex];
  }
  return NULL;
}

/* USART device endpoints */

/**
 * @brief   IN EP1 state.
 */
static USBInEndpointState ep1instate;

/**
 * @brief   OUT EP1 state.
 */
static USBOutEndpointState ep1outstate;

/**
 * @brief   EP1 initialization structure (both IN and OUT).
 */
static const USBEndpointConfig ep1config = {
  USB_EP_MODE_TYPE_BULK,
  NULL,
  sduDataTransmitted,
  sduDataReceived,
  0x0040,
  0x0040,
  &ep1instate,
  &ep1outstate,
  2,
  NULL
};

/**
 * @brief   IN EP2 state.
 */
static USBInEndpointState ep2instate;

/**
 * @brief   EP2 initialization structure (IN only).
 */
static const USBEndpointConfig ep2config = {
  USB_EP_MODE_TYPE_INTR,
  NULL,
  sduInterruptTransmitted,
  NULL,
  0x0010,
  0x0000,
  &ep2instate,
  NULL,
  1,
  NULL
};


/* HID device endpoints */

static USBInEndpointState ep3instate;
//static USBOutEndpointState ep1outstate;
//      EndPoint Initialization. INTERRUPT IN. Device -> Host
static const USBEndpointConfig ep3config = {
        USB_EP_MODE_TYPE_INTR,
        NULL,
        hidDataTransmitted,
        NULL,
        0x0004,
        0x0000,
        &ep3instate,
        NULL,
        1,
        NULL
};

static USBOutEndpointState ep4outstate;
//static USBOutEndpointState ep1outstate;
//      EndPoint Initialization. INTERRUPT IN. Device -> Host
static const USBEndpointConfig ep4config = {
        USB_EP_MODE_TYPE_INTR,
        NULL,
    NULL,
        hidDataReceived,
        0x0000,
        0x0004,
        NULL,
        &ep4outstate,
        1,
        NULL
};

/*
 * Handles the USB driver global events.
 */
static void usb_event(USBDriver *usbp, usbevent_t event) {
    extern SerialUSBDriver SDU1;

    switch(event) {
    case USB_EVENT_RESET:
        return;
    case USB_EVENT_ADDRESS:
        return;
    case USB_EVENT_CONFIGURED:
        chSysLockFromISR();

        /* Enables the endpoints specified into the configuration.
           Note, this callback is invoked from an ISR so I-Class functions
           must be used.*/
        usbInitEndpointI(usbp, USBD1_DATA_REQUEST_EP, &ep1config);
        usbInitEndpointI(usbp, USBD1_INTERRUPT_REQUEST_EP, &ep2config);

        usbInitEndpointI(usbp, HID_IN_EP_ADDRESS, &ep3config);
        usbInitEndpointI(usbp, HID_OUT_EP_ADDRESS, &ep4config);

        /* Resetting the state of the CDC subsystem.*/
        sduConfigureHookI(&SDU1);

        chSysUnlockFromISR();
        return;
    case USB_EVENT_SUSPEND:
        return;
    case USB_EVENT_WAKEUP:
        return;
    case USB_EVENT_STALLED:
        return;
    }
    return;
}

/*
 * Handles the USB driver global events.
 */
static void sof_handler(USBDriver *usbp) {

  (void)usbp;

  osalSysLockFromISR();
  sduSOFHookI(&SDU1);
  osalSysUnlockFromISR();
}

const USBConfig usbcfg = {
        usb_event,
        get_descriptor,
        hidRequestsHook,
        sof_handler
};

/*
 * Serial over USB driver configuration.
 */
const SerialUSBConfig serusbcfg = {
  &USBD1,
  USBD1_DATA_REQUEST_EP,
  USBD1_DATA_AVAILABLE_EP,
  USBD1_INTERRUPT_REQUEST_EP
};

