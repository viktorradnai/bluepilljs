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
#include "usbcfg.h"

hid_data hid_in_data = { 0, 0 };
hid_data hid_out_data = { 0, 0 };
uint8_t usbInitState = 0;


void hid_receive(USBDriver *usbp)
{
    chSysLockFromISR();
    usbStartReceiveI(usbp, HID_OUT_EP_ADDRESS, (uint8_t *)&hid_out_data, sizeof (hid_out_data));
    chSysUnlockFromISR();
}

void hid_transmit(USBDriver *usbp)
{
    chSysLockFromISR();
    usbStartTransmitI(usbp, HID_IN_EP_ADDRESS, (uint8_t *)&hid_in_data, sizeof (hid_in_data));
    chSysUnlockFromISR();
}

bool hidRequestsHook(USBDriver *usbp)
{
    const USBDescriptor *dp;
    if ((usbp->setup[0] & (USB_RTYPE_TYPE_MASK | USB_RTYPE_RECIPIENT_MASK)) ==
        (USB_RTYPE_TYPE_STD | USB_RTYPE_RECIPIENT_INTERFACE)) {
        switch (usbp->setup[1]) {
        case USB_REQ_GET_DESCRIPTOR:
            dp = usbp->config->get_descriptor_cb(
            usbp, usbp->setup[3], usbp->setup[2],
            usbFetchWord(&usbp->setup[4]));
            if (dp == NULL)
                return FALSE;
            usbSetupTransfer(usbp, (uint8_t *)dp->ud_string, dp->ud_size, NULL);
            return TRUE;
        default:
            return FALSE;
        }
    }

    if ((usbp->setup[0] & (USB_RTYPE_TYPE_MASK | USB_RTYPE_RECIPIENT_MASK)) ==
        (USB_RTYPE_TYPE_CLASS | USB_RTYPE_RECIPIENT_INTERFACE)) {
        switch (usbp->setup[1]) {
        case HID_GET_REPORT_REQUEST:
            palSetPadMode(GPIOE, 12, PAL_MODE_OUTPUT_PUSHPULL);
            palSetPad(GPIOE, 12);
            hid_in_data.x = 0;
            hid_in_data.button = 0;
            usbSetupTransfer(usbp, (uint8_t *) &hid_in_data, sizeof(hid_in_data), NULL);
            usbInitState = 1;
            return TRUE;
        case HID_GET_IDLE_REQUEST:
            usbSetupTransfer(usbp, NULL, 0, NULL);
            return TRUE;
        case HID_GET_PROTOCOL_REQUEST:
            return TRUE;
        case HID_SET_REPORT_REQUEST:
            usbSetupTransfer(usbp, NULL, 0, NULL);
            return TRUE;
        case HID_SET_IDLE_REQUEST:
            usbSetupTransfer(usbp, NULL, 0, NULL);
            return TRUE;
        case HID_SET_PROTOCOL_REQUEST:
            return TRUE;
        default:
            return FALSE;
        }
    }

    if ((usbp->setup[0] & USB_RTYPE_TYPE_MASK) == USB_RTYPE_TYPE_CLASS) {
        return sduRequestsHook(usbp);
    }
    return FALSE;
}


// Callback for THE IN ENDPOINT (INTERRUPT). Device -> HOST
void hidDataTransmitted(USBDriver *usbp, usbep_t ep)
{
    (void)usbp;
    (void)ep;
}


// Callback for THE OUT ENDPOINT (INTERRUPT)
void hidDataReceived(USBDriver *usbp, usbep_t ep)
{
    (void)usbp;
    (void)ep;
}




