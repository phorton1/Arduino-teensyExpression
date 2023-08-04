// prh - Heavily Edited

/* Teensyduino Core Library
 * http://www.pjrc.com/teensy/
 * Copyright (c) 2017 PJRC.COM, LLC.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * 1. The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * 2. If the Software is incorporated into a build system that allows
 * selection among a list of target devices, then similar target
 * devices manufactured by PJRC.COM must be included in the list of
 * target devices and selectable in the same manner.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#if F_CPU >= 20000000

#define USB_DESC_LIST_DEFINE
#include "usb_desc_prh.h"
#ifdef NUM_ENDPOINTS
#include "usb_names.h"
#include "kinetis.h"
#include "avr_functions.h"

#define iProductOffset   15
    // bytes to iProduct within device_descriptor
#define uniqueIDOffset  10
    // bump this byte in the device descriptor for the fishman

//------------------------------------------------------
// USB Device - unchanged
//------------------------------------------------------

#define LSB(n) ((n) & 255)
#define MSB(n) (((n) >> 8) & 255)


static uint8_t device_descriptor[] = {
        18,                                     // bLength
        1,                                      // bDescriptorType
        0x10, 0x01,                             // bcdUSB
#ifdef DEVICE_CLASS
        DEVICE_CLASS,                           // bDeviceClass
#else
	0,
#endif
#ifdef DEVICE_SUBCLASS
        DEVICE_SUBCLASS,                        // bDeviceSubClass
#else
	0,
#endif
#ifdef DEVICE_PROTOCOL
        DEVICE_PROTOCOL,                        // bDeviceProtocol
#else
	0,
#endif
        EP0_SIZE,                               // bMaxPacketSize0
        LSB(VENDOR_ID), MSB(VENDOR_ID),         // idVendor
        LSB(PRODUCT_ID), MSB(PRODUCT_ID),       // idProduct
#ifdef BCD_DEVICE
        LSB(BCD_DEVICE), MSB(BCD_DEVICE),       // bcdDevice
#else
  // For USB types that don't explicitly define BCD_DEVICE,
  // use the minor version number to help teensy_ports
  // identify which Teensy model is used.
  #if defined(__MKL26Z64__)
        0x73, 0x02,
  #elif defined(__MK20DX128__)
        0x74, 0x02,
  #elif defined(__MK20DX256__)
        0x75, 0x02,
  #elif defined(__MK64FX512__)
        0x76, 0x02,
  #elif defined(__MK66FX1M0__)
        0x77, 0x02,
  #else
        0x00, 0x02,
  #endif
#endif
        1,                                      // iManufacturer
        2,                                      // iProduct
        3,                                      // iSerialNumber
        1                                       // bNumConfigurations
};



//--------------------------------------------------
// USB Descriptor Size - comes in two flavors
//--------------------------------------------------

#define CONFIG_HEADER_DESCRIPTOR_SIZE	9

#define CDC_IAD_DESCRIPTOR_POS		CONFIG_HEADER_DESCRIPTOR_SIZE
#define CDC_IAD_DESCRIPTOR_SIZE		8

#define CDC_DATA_INTERFACE_DESC_POS	CDC_IAD_DESCRIPTOR_POS+CDC_IAD_DESCRIPTOR_SIZE
#define CDC_DATA_INTERFACE_DESC_SIZE	9+5+5+4+5+7+9+7+7

#define MIDI_INTERFACE_DESC_POS		CDC_DATA_INTERFACE_DESC_POS+CDC_DATA_INTERFACE_DESC_SIZE

#define CONFIG1_NUM_CABLES  2
#define CONFIG2_NUM_CABLES  2

#define MIDI_INTERFACE1_DESC_SIZE	9+7+((6+6+9+9)* CONFIG1_NUM_CABLES)+(9+4+CONFIG1_NUM_CABLES)*2
#define MIDI_INTERFACE2_DESC_SIZE	9+7+((6+6+9+9)* CONFIG2_NUM_CABLES)+(9+4+CONFIG2_NUM_CABLES)*2

#define CONFIG1_DESC_SIZE		MIDI_INTERFACE_DESC_POS+MIDI_INTERFACE1_DESC_SIZE
#define CONFIG2_DESC_SIZE		MIDI_INTERFACE_DESC_POS+MIDI_INTERFACE2_DESC_SIZE


#define MIDI_INTERFACE_JACK_PAIR(a, b, c, d, ij, oj) \
    6, 0x24, 0x02, 0x01, (a), (ij), \
    6, 0x24, 0x02, 0x02, (b), (ij), \
    9, 0x24, 0x03, 0x01, (c), 1, (b), 1, (oj), \
    9, 0x24, 0x03, 0x02, (d), 1, (a), 1, (oj),


// prh - doesnt work
// uint16_t MIDI_NUM_CABLES = CONFIG1_NUM_CABLES;


//------------------------------------------------------------------------------
//   USB Configuration1 - One Midi Cable - teensyExpression
//------------------------------------------------------------------------------
// There are two complete copies of the config descriptor
// The pointer and size of the active one is set at runtime ...

static uint8_t config1_descriptor[CONFIG1_DESC_SIZE] =            // ONE MIDI CABLE
{
        // configuration descriptor, USB spec 9.6.3, page 264-266, Table 9-10
        9,                                      // bLength;
        2,                                      // bDescriptorType;
        LSB(CONFIG1_DESC_SIZE),                 // wTotalLength
        MSB(CONFIG1_DESC_SIZE),
        NUM_INTERFACE,                          // bNumInterfaces
        1,                                      // bConfigurationValue
        0,                                      // iConfiguration
        0xC0,                                   // bmAttributes
        50,                                     // bMaxPower

// #ifdef CDC_IAD_DESCRIPTOR
        // interface association descriptor, USB ECN, Table 9-Z
        8,                                      // bLength
        11,                                     // bDescriptorType
        CDC_STATUS_INTERFACE,                   // bFirstInterface
        2,                                      // bInterfaceCount
        0x02,                                   // bFunctionClass
        0x02,                                   // bFunctionSubClass
        0x01,                                   // bFunctionProtocol
        4,                                      // iFunction
// #endif

// #ifdef CDC_DATA_INTERFACE
        // interface descriptor, USB spec 9.6.5, page 267-269, Table 9-12
        9,                                      // bLength
        4,                                      // bDescriptorType
        CDC_STATUS_INTERFACE,			// bInterfaceNumber
        0,                                      // bAlternateSetting
        1,                                      // bNumEndpoints
        0x02,                                   // bInterfaceClass
        0x02,                                   // bInterfaceSubClass
        0x01,                                   // bInterfaceProtocol
        0,                                      // iInterface
        // CDC Header Functional Descriptor, CDC Spec 5.2.3.1, Table 26
        5,                                      // bFunctionLength
        0x24,                                   // bDescriptorType
        0x00,                                   // bDescriptorSubtype
        0x10, 0x01,                             // bcdCDC
        // Call Management Functional Descriptor, CDC Spec 5.2.3.2, Table 27
        5,                                      // bFunctionLength
        0x24,                                   // bDescriptorType
        0x01,                                   // bDescriptorSubtype
        0x01,                                   // bmCapabilities
        1,                                      // bDataInterface
        // Abstract Control Management Functional Descriptor, CDC Spec 5.2.3.3, Table 28
        4,                                      // bFunctionLength
        0x24,                                   // bDescriptorType
        0x02,                                   // bDescriptorSubtype
        0x06,                                   // bmCapabilities
        // Union Functional Descriptor, CDC Spec 5.2.3.8, Table 33
        5,                                      // bFunctionLength
        0x24,                                   // bDescriptorType
        0x06,                                   // bDescriptorSubtype
        CDC_STATUS_INTERFACE,                   // bMasterInterface
        CDC_DATA_INTERFACE,                     // bSlaveInterface0
        // endpoint descriptor, USB spec 9.6.6, page 269-271, Table 9-13
        7,                                      // bLength
        5,                                      // bDescriptorType
        CDC_ACM_ENDPOINT | 0x80,                // bEndpointAddress
        0x03,                                   // bmAttributes (0x03=intr)
        CDC_ACM_SIZE, 0,                        // wMaxPacketSize
        64,                                     // bInterval
        // interface descriptor, USB spec 9.6.5, page 267-269, Table 9-12
        9,                                      // bLength
        4,                                      // bDescriptorType
        CDC_DATA_INTERFACE,                     // bInterfaceNumber
        0,                                      // bAlternateSetting
        2,                                      // bNumEndpoints
        0x0A,                                   // bInterfaceClass
        0x00,                                   // bInterfaceSubClass
        0x00,                                   // bInterfaceProtocol
        0,                                      // iInterface
        // endpoint descriptor, USB spec 9.6.6, page 269-271, Table 9-13
        7,                                      // bLength
        5,                                      // bDescriptorType
        CDC_RX_ENDPOINT,                        // bEndpointAddress
        0x02,                                   // bmAttributes (0x02=bulk)
        CDC_RX_SIZE, 0,                         // wMaxPacketSize
        0,                                      // bInterval
        // endpoint descriptor, USB spec 9.6.6, page 269-271, Table 9-13
        7,                                      // bLength
        5,                                      // bDescriptorType
        CDC_TX_ENDPOINT | 0x80,                 // bEndpointAddress
        0x02,                                   // bmAttributes (0x02=bulk)
        CDC_TX_SIZE, 0,                         // wMaxPacketSize
        0,                                      // bInterval
// #endif // CDC_DATA_INTERFACE

// #ifdef MIDI_INTERFACE
        // Standard MS Interface Descriptor,
        9,                                      // bLength
        4,                                      // bDescriptorType
        MIDI_INTERFACE,                         // bInterfaceNumber
        0,                                      // bAlternateSetting
        2,                                      // bNumEndpoints
        0x01,                                   // bInterfaceClass (0x01 = Audio)
        0x03,                                   // bInterfaceSubClass (0x03 = MIDI)
        0x00,                                   // bInterfaceProtocol (unused for MIDI)
        0,                                      // iInterface
        // MIDI MS Interface Header, USB MIDI 6.1.2.1, page 21, Table 6-2
        7,                                      // bLength
        0x24,                                   // bDescriptorType = CS_INTERFACE
        0x01,                                   // bDescriptorSubtype = MS_HEADER
        0x00, 0x01,                             // bcdMSC = revision 01.00
        LSB(7+(6+6+9+9)*CONFIG1_NUM_CABLES),    // wTotalLength
        MSB(7+(6+6+9+9)*CONFIG1_NUM_CABLES),

        // MIDI IN Jack Descriptor, B.4.3, Table B-7 (embedded), page 40
        6,                                      // bLength
        0x24,                                   // bDescriptorType = CS_INTERFACE
        0x02,                                   // bDescriptorSubtype = MIDI_IN_JACK
        0x01,                                   // bJackType = EMBEDDED
        1,                                      // bJackID, ID = 1
        4,                                      // prh - iJack
        // MIDI IN Jack Descriptor, B.4.3, Table B-8 (external), page 40
        6,                                      // bLength
        0x24,                                   // bDescriptorType = CS_INTERFACE
        0x02,                                   // bDescriptorSubtype = MIDI_IN_JACK
        0x02,                                   // bJackType = EXTERNAL
        2,                                      // bJackID, ID = 2
        4,                                      // prh - iJack
        // MIDI OUT Jack Descriptor, B.4.4, Table B-9, page 41
        9,
        0x24,                                   // bDescriptorType = CS_INTERFACE
        0x03,                                   // bDescriptorSubtype = MIDI_OUT_JACK
        0x01,                                   // bJackType = EMBEDDED
        3,                                      // bJackID, ID = 3
        1,                                      // bNrInputPins = 1 pin
        2,                                      // BaSourceID(1) = 2
        1,                                      // BaSourcePin(1) = first pin
        5,                                      // prh - iJack
        // MIDI OUT Jack Descriptor, B.4.4, Table B-10, page 41
        9,
        0x24,                                   // bDescriptorType = CS_INTERFACE
        0x03,                                   // bDescriptorSubtype = MIDI_OUT_JACK
        0x02,                                   // bJackType = EXTERNAL
        4,                                      // bJackID, ID = 4
        1,                                      // bNrInputPins = 1 pin
        1,                                      // BaSourceID(1) = 1
        1,                                      // BaSourcePin(1) = first pin
        5,                                      // prh - iJack

        MIDI_INTERFACE_JACK_PAIR(5, 6, 7, 8, 6, 7)
            // one extra cable

        // Standard Bulk OUT Endpoint Descriptor, B.5.1, Table B-11, pae 42
        9,                                      // bLength
        5,                                      // bDescriptorType = ENDPOINT
        MIDI_RX_ENDPOINT,                       // bEndpointAddress
        0x02,                                   // bmAttributes (0x02=bulk)
        MIDI_RX_SIZE, 0,                        // wMaxPacketSize
        0,                                      // bInterval
        0,                                      // bRefresh
        0,                                      // bSynchAddress
        // Class-specific MS Bulk OUT Endpoint Descriptor, B.5.2, Table B-12, page 42
        4+CONFIG1_NUM_CABLES,                      // bLength
        0x25,                                   // bDescriptorSubtype = CS_ENDPOINT
        0x01,                                   // bJackType = MS_GENERAL
        CONFIG1_NUM_CABLES,                        // bNumEmbMIDIJack = number of jacks
        1,                                      // BaAssocJackID(1) = jack ID #1

        5,  // prh - one extra byte here for one extra cable

        // Standard Bulk IN Endpoint Descriptor, B.5.1, Table B-11, pae 42
        9,                                      // bLength
        5,                                      // bDescriptorType = ENDPOINT
        MIDI_TX_ENDPOINT | 0x80,                // bEndpointAddress
        0x02,                                   // bmAttributes (0x02=bulk)
        MIDI_TX_SIZE, 0,                        // wMaxPacketSize
        0,                                      // bInterval
        0,                                      // bRefresh
        0,                                      // bSynchAddress
        // Class-specific MS Bulk IN Endpoint Descriptor, B.5.2, Table B-12, page 42
        4+CONFIG1_NUM_CABLES,                      // bLength
        0x25,                                   // bDescriptorSubtype = CS_ENDPOINT
        0x01,                                   // bJackType = MS_GENERAL
        CONFIG1_NUM_CABLES,                        // bNumEmbMIDIJack = number of jacks
        3,                                      // BaAssocJackID(1) = jack ID #3

        7,      // prh - one extra byte here for one extra cable

// #endif // MIDI_INTERFACE

};  // end of config1_desxcriptor (teensyExpression)



//------------------------------------------------------------------------------
//   USB Configuration2 - Two Midi Cables - FTP
//------------------------------------------------------------------------------

static uint8_t config2_descriptor[CONFIG2_DESC_SIZE] =   // TWO MIDI_CABLES
{
        // configuration descriptor, USB spec 9.6.3, page 264-266, Table 9-10
        9,                                      // bLength;
        2,                                      // bDescriptorType;
        LSB(CONFIG2_DESC_SIZE),                 // wTotalLength
        MSB(CONFIG2_DESC_SIZE),
        NUM_INTERFACE,                          // bNumInterfaces
        1,                                      // bConfigurationValue
        0,                                      // iConfiguration
        0xC0,                                   // bmAttributes
        50,                                     // bMaxPower

// #ifdef CDC_IAD_DESCRIPTOR
        // interface association descriptor, USB ECN, Table 9-Z
        8,                                      // bLength
        11,                                     // bDescriptorType
        CDC_STATUS_INTERFACE,                   // bFirstInterface
        2,                                      // bInterfaceCount
        0x02,                                   // bFunctionClass
        0x02,                                   // bFunctionSubClass
        0x01,                                   // bFunctionProtocol
        4,                                      // iFunction
// #endif

// #ifdef CDC_DATA_INTERFACE
        // interface descriptor, USB spec 9.6.5, page 267-269, Table 9-12
        9,                                      // bLength
        4,                                      // bDescriptorType
        CDC_STATUS_INTERFACE,			// bInterfaceNumber
        0,                                      // bAlternateSetting
        1,                                      // bNumEndpoints
        0x02,                                   // bInterfaceClass
        0x02,                                   // bInterfaceSubClass
        0x01,                                   // bInterfaceProtocol
        0,                                      // iInterface
        // CDC Header Functional Descriptor, CDC Spec 5.2.3.1, Table 26
        5,                                      // bFunctionLength
        0x24,                                   // bDescriptorType
        0x00,                                   // bDescriptorSubtype
        0x10, 0x01,                             // bcdCDC
        // Call Management Functional Descriptor, CDC Spec 5.2.3.2, Table 27
        5,                                      // bFunctionLength
        0x24,                                   // bDescriptorType
        0x01,                                   // bDescriptorSubtype
        0x01,                                   // bmCapabilities
        1,                                      // bDataInterface
        // Abstract Control Management Functional Descriptor, CDC Spec 5.2.3.3, Table 28
        4,                                      // bFunctionLength
        0x24,                                   // bDescriptorType
        0x02,                                   // bDescriptorSubtype
        0x06,                                   // bmCapabilities
        // Union Functional Descriptor, CDC Spec 5.2.3.8, Table 33
        5,                                      // bFunctionLength
        0x24,                                   // bDescriptorType
        0x06,                                   // bDescriptorSubtype
        CDC_STATUS_INTERFACE,                   // bMasterInterface
        CDC_DATA_INTERFACE,                     // bSlaveInterface0
        // endpoint descriptor, USB spec 9.6.6, page 269-271, Table 9-13
        7,                                      // bLength
        5,                                      // bDescriptorType
        CDC_ACM_ENDPOINT | 0x80,                // bEndpointAddress
        0x03,                                   // bmAttributes (0x03=intr)
        CDC_ACM_SIZE, 0,                        // wMaxPacketSize
        64,                                     // bInterval
        // interface descriptor, USB spec 9.6.5, page 267-269, Table 9-12
        9,                                      // bLength
        4,                                      // bDescriptorType
        CDC_DATA_INTERFACE,                     // bInterfaceNumber
        0,                                      // bAlternateSetting
        2,                                      // bNumEndpoints
        0x0A,                                   // bInterfaceClass
        0x00,                                   // bInterfaceSubClass
        0x00,                                   // bInterfaceProtocol
        0,                                      // iInterface
        // endpoint descriptor, USB spec 9.6.6, page 269-271, Table 9-13
        7,                                      // bLength
        5,                                      // bDescriptorType
        CDC_RX_ENDPOINT,                        // bEndpointAddress
        0x02,                                   // bmAttributes (0x02=bulk)
        CDC_RX_SIZE, 0,                         // wMaxPacketSize
        0,                                      // bInterval
        // endpoint descriptor, USB spec 9.6.6, page 269-271, Table 9-13
        7,                                      // bLength
        5,                                      // bDescriptorType
        CDC_TX_ENDPOINT | 0x80,                 // bEndpointAddress
        0x02,                                   // bmAttributes (0x02=bulk)
        CDC_TX_SIZE, 0,                         // wMaxPacketSize
        0,                                      // bInterval
// #endif // CDC_DATA_INTERFACE

// #ifdef MIDI_INTERFACE
        // Standard MS Interface Descriptor,
        9,                                      // bLength
        4,                                      // bDescriptorType
        MIDI_INTERFACE,                         // bInterfaceNumber
        0,                                      // bAlternateSetting
        2,                                      // bNumEndpoints
        0x01,                                   // bInterfaceClass (0x01 = Audio)
        0x03,                                   // bInterfaceSubClass (0x03 = MIDI)
        0x00,                                   // bInterfaceProtocol (unused for MIDI)
        0,                                      // iInterface
        // MIDI MS Interface Header, USB MIDI 6.1.2.1, page 21, Table 6-2
        7,                                      // bLength
        0x24,                                   // bDescriptorType = CS_INTERFACE
        0x01,                                   // bDescriptorSubtype = MS_HEADER
        0x00, 0x01,                             // bcdMSC = revision 01.00
        LSB(7+(6+6+9+9)*CONFIG2_NUM_CABLES),    // wTotalLength
        MSB(7+(6+6+9+9)*CONFIG2_NUM_CABLES),

        // MIDI IN Jack Descriptor, B.4.3, Table B-7 (embedded), page 40
        6,                                      // bLength
        0x24,                                   // bDescriptorType = CS_INTERFACE
        0x02,                                   // bDescriptorSubtype = MIDI_IN_JACK
        0x01,                                   // bJackType = EMBEDDED
        1,                                      // bJackID, ID = 1
        8,                                      // prh - iJack
        // MIDI IN Jack Descriptor, B.4.3, Table B-8 (external), page 40
        6,                                      // bLength
        0x24,                                   // bDescriptorType = CS_INTERFACE
        0x02,                                   // bDescriptorSubtype = MIDI_IN_JACK
        0x02,                                   // bJackType = EXTERNAL
        2,                                      // bJackID, ID = 2
        8,                                      // prh - iJack
        // MIDI OUT Jack Descriptor, B.4.4, Table B-9, page 41
        9,
        0x24,                                   // bDescriptorType = CS_INTERFACE
        0x03,                                   // bDescriptorSubtype = MIDI_OUT_JACK
        0x01,                                   // bJackType = EMBEDDED
        3,                                      // bJackID, ID = 3
        1,                                      // bNrInputPins = 1 pin
        2,                                      // BaSourceID(1) = 2
        1,                                      // BaSourcePin(1) = first pin
        9,                                      // prh - iJack
        // MIDI OUT Jack Descriptor, B.4.4, Table B-10, page 41
        9,
        0x24,                                   // bDescriptorType = CS_INTERFACE
        0x03,                                   // bDescriptorSubtype = MIDI_OUT_JACK
        0x02,                                   // bJackType = EXTERNAL
        4,                                      // bJackID, ID = 4
        1,                                      // bNrInputPins = 1 pin
        1,                                      // BaSourceID(1) = 1
        1,                                      // BaSourcePin(1) = first pin
        9,                                      // prh - iJack

        MIDI_INTERFACE_JACK_PAIR(5, 6, 7, 8, 0xA, 0xB)
            // one extra cable

        // Standard Bulk OUT Endpoint Descriptor, B.5.1, Table B-11, pae 42
        9,                                      // bLength
        5,                                      // bDescriptorType = ENDPOINT
        MIDI_RX_ENDPOINT,                       // bEndpointAddress
        0x02,                                   // bmAttributes (0x02=bulk)
        MIDI_RX_SIZE, 0,                        // wMaxPacketSize
        0,                                      // bInterval
        0,                                      // bRefresh
        0,                                      // bSynchAddress
        // Class-specific MS Bulk OUT Endpoint Descriptor, B.5.2, Table B-12, page 42
        4+CONFIG2_NUM_CABLES,                      // bLength
        0x25,                                   // bDescriptorSubtype = CS_ENDPOINT
        0x01,                                   // bJackType = MS_GENERAL
        CONFIG2_NUM_CABLES,                        // bNumEmbMIDIJack = number of jacks
        1,                                      // BaAssocJackID(1) = jack ID #1

        5,  // prh - one extra byte here for one extra cable

        // Standard Bulk IN Endpoint Descriptor, B.5.1, Table B-11, pae 42
        9,                                      // bLength
        5,                                      // bDescriptorType = ENDPOINT
        MIDI_TX_ENDPOINT | 0x80,                // bEndpointAddress
        0x02,                                   // bmAttributes (0x02=bulk)
        MIDI_TX_SIZE, 0,                        // wMaxPacketSize
        0,                                      // bInterval
        0,                                      // bRefresh
        0,                                      // bSynchAddress
        // Class-specific MS Bulk IN Endpoint Descriptor, B.5.2, Table B-12, page 42
        4+CONFIG2_NUM_CABLES,                      // bLength
        0x25,                                   // bDescriptorSubtype = CS_ENDPOINT
        0x01,                                   // bJackType = MS_GENERAL
        CONFIG2_NUM_CABLES,                        // bNumEmbMIDIJack = number of jacks
        3,                                      // BaAssocJackID(1) = jack ID #3

         7,      // prh - one extra byte here for one extra cable

            // finished

// #endif // MIDI_INTERFACE

};



// **************************************************************
//   String Descriptors
// **************************************************************

// The descriptors above can provide human readable strings,
// referenced by index numbers.  These descriptors are the
// actual string data

/* defined in usb_names.h
struct usb_string_descriptor_struct {
        uint8_t bLength;
        uint8_t bDescriptorType;
        uint16_t wString[];
};
*/

extern struct usb_string_descriptor_struct usb_string_manufacturer_name
        __attribute__ ((weak, alias("usb_string_manufacturer_name_default")));
extern struct usb_string_descriptor_struct usb_string_product_name
        __attribute__ ((weak, alias("usb_string_product_name_default")));
extern struct usb_string_descriptor_struct usb_string_serial_number
        __attribute__ ((weak, alias("usb_string_serial_number_default")));


struct usb_string_descriptor_struct string0 = {
        4,
        3,
        {0x0409}
};

struct usb_string_descriptor_struct usb_string_manufacturer_name_default = {
        2 + MANUFACTURER_NAME_LEN * 2,
        3,
        MANUFACTURER_NAME
};

struct usb_string_descriptor_struct usb_string_product_name_default = {
	2 + PRODUCT_NAME_LEN * 2,
        3,
        PRODUCT_NAME
};

struct usb_string_descriptor_struct usb_string_serial_number_default = {
        12,
        3,
        {0,0,0,0,0,0,0,0,0,0}
};




void usb_init_serialnumber(void)
    // vestigal placement of code to access usb_string_serial_number_default
{
	char buf[11];
	uint32_t i, num;

	__disable_irq();
#if defined(HAS_KINETIS_FLASH_FTFA) || defined(HAS_KINETIS_FLASH_FTFL)
	FTFL_FSTAT = FTFL_FSTAT_RDCOLERR | FTFL_FSTAT_ACCERR | FTFL_FSTAT_FPVIOL;
	FTFL_FCCOB0 = 0x41;
	FTFL_FCCOB1 = 15;
	FTFL_FSTAT = FTFL_FSTAT_CCIF;
	while (!(FTFL_FSTAT & FTFL_FSTAT_CCIF)) ; // wait
	num = *(uint32_t *)&FTFL_FCCOB7;
#elif defined(HAS_KINETIS_FLASH_FTFE)
	kinetis_hsrun_disable();
	FTFL_FSTAT = FTFL_FSTAT_RDCOLERR | FTFL_FSTAT_ACCERR | FTFL_FSTAT_FPVIOL;
	*(uint32_t *)&FTFL_FCCOB3 = 0x41070000;
	FTFL_FSTAT = FTFL_FSTAT_CCIF;
	while (!(FTFL_FSTAT & FTFL_FSTAT_CCIF)) ; // wait
	num = *(uint32_t *)&FTFL_FCCOBB;
	kinetis_hsrun_enable();
#endif
	__enable_irq();
	// add extra zero to work around OS-X CDC-ACM driver bug
	if (num < 10000000) num = num * 10;
	ultoa(num, buf, 10);
	for (i=0; i<10; i++) {
		char c = buf[i];
		if (!c) break;
		usb_string_serial_number_default.wString[i] = c;
	}
	usb_string_serial_number_default.bLength = i * 2 + 2;
}




struct usb_string_descriptor_struct   jack00_name = {
        2 + 21 * 2,
        3,
        {'t','e','e','n','s','y','E','x','p','r','e','s','s','i','o','n',' ','(','i','n',')'}
};
struct usb_string_descriptor_struct   jack01_name = {
        2 + 22 * 2,
        3,
        {'t','e','e','n','s','y','E','x','p','r','e','s','s','i','o','n',' ','(','o','u','t',')'}
};
struct usb_string_descriptor_struct   jack10_name = {
        2 + 18 * 2,
        3,
        {'t','e','e','n','s','y','C','o','n','t','r','o','l',' ','(','i','n',')'}
};
struct usb_string_descriptor_struct   jack11_name = {
        2 + 19 * 2,
        3,
        {'t','e','e','n','s','y','C','o','n','t','r','o','l',' ','(','o','u','t',')'}
};



struct usb_string_descriptor_struct   jack20_name = {
        2 + 18 * 2,
        3,
        {'F','i','s','h','m','a','n',' ','T','r','i','p','l','e','P','l','a','y'}
};
struct usb_string_descriptor_struct   jack21_name = {
        2 + 18 * 2,
        3,
        {'F','i','s','h','m','a','n',' ','T','r','i','p','l','e','P','l','a','y'}
};
struct usb_string_descriptor_struct   jack30_name = {
        2 + 28 * 2,
        3,
        {'M','I','D','I','I','N','2',' ','(','F','i','s','h','m','a','n',' ','T','r','i','p','l','e','P','l','a','y',')'}
};
struct usb_string_descriptor_struct   jack31_name = {
        2 + 29 * 2,
        3,
        {'M','I','D','I','O','U','T','2',' ','(','F','i','s','h','m','a','n',' ','T','r','i','p','l','e','P','l','a','y',')'}
};
struct usb_string_descriptor_struct   FTP_product_name = {
        2 + 18 * 2,
        3,
        {'F','i','s','h','m','a','n',' ','T','r','i','p','l','e','P','l','a','y'}
};





// **************************************************************
//   Descriptors List
// **************************************************************

// This table provides access to all the descriptor data above.

// prh - removed the const

usb_descriptor_list_t usb_descriptor_list[] = {
	//wValue, wIndex, address,          length
	{0x0100, 0x0000, device_descriptor,  sizeof(device_descriptor)},
	{0x0200, 0x0000, config1_descriptor, CONFIG1_DESC_SIZE},

    {0x0300, 0x0000, (const uint8_t *)&string0, 0},
    {0x0301, 0x0409, (const uint8_t *)&usb_string_manufacturer_name, 0},
    {0x0302, 0x0409, (const uint8_t *)&usb_string_product_name, 0},
    {0x0303, 0x0409, (const uint8_t *)&usb_string_serial_number, 0},

    {0x0304, 0x0409, (const uint8_t *)&jack00_name, 0},
    {0x0305, 0x0409, (const uint8_t *)&jack01_name, 0},
    {0x0306, 0x0409, (const uint8_t *)&jack10_name, 0},
    {0x0307, 0x0409, (const uint8_t *)&jack11_name, 0},
    {0x0308, 0x0409, (const uint8_t *)&jack20_name, 0},
    {0x0309, 0x0409, (const uint8_t *)&jack21_name, 0},
    {0x030A, 0x0409, (const uint8_t *)&jack30_name, 0},
    {0x030B, 0x0409, (const uint8_t *)&jack31_name, 0},
    {0x030C, 0x0409, (const uint8_t *)&FTP_product_name, 0},

	{0, 0, NULL, 0}
};




void setFishmanFTPDescriptor()
{
    // prh - doesnt work
    // MIDI_NUM_CABLES = CONFIG2_NUM_CABLES;

    // bump the product vid so you don't have to uninstall

    device_descriptor[uniqueIDOffset]++;

    // set the stringId into the device descriptor

    device_descriptor[iProductOffset] = 0x0c;     // FTP_product_name string index

    // set the pointer and size of the config desxcriptor

    usb_descriptor_list[1].addr = config2_descriptor;
    usb_descriptor_list[1].length = CONFIG2_DESC_SIZE;
}



// **************************************************************
//   Endpoint Configuration
// **************************************************************

#if 0
// 0x00 = not used
// 0x19 = Recieve only
// 0x15 = Transmit only
// 0x1D = Transmit & Recieve
//
const uint8_t usb_endpoint_config_table[NUM_ENDPOINTS] =
{
	0x00, 0x15, 0x19, 0x15, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};
#endif


const uint8_t usb_endpoint_config_table[NUM_ENDPOINTS] =
{
#if (defined(ENDPOINT1_CONFIG) && NUM_ENDPOINTS >= 1)
	ENDPOINT1_CONFIG,
#elif (NUM_ENDPOINTS >= 1)
	ENDPOINT_UNUSED,
#endif
#if (defined(ENDPOINT2_CONFIG) && NUM_ENDPOINTS >= 2)
	ENDPOINT2_CONFIG,
#elif (NUM_ENDPOINTS >= 2)
	ENDPOINT_UNUSED,
#endif
#if (defined(ENDPOINT3_CONFIG) && NUM_ENDPOINTS >= 3)
	ENDPOINT3_CONFIG,
#elif (NUM_ENDPOINTS >= 3)
	ENDPOINT_UNUSED,
#endif
#if (defined(ENDPOINT4_CONFIG) && NUM_ENDPOINTS >= 4)
	ENDPOINT4_CONFIG,
#elif (NUM_ENDPOINTS >= 4)
	ENDPOINT_UNUSED,
#endif
#if (defined(ENDPOINT5_CONFIG) && NUM_ENDPOINTS >= 5)
	ENDPOINT5_CONFIG,
#elif (NUM_ENDPOINTS >= 5)
	ENDPOINT_UNUSED,
#endif
#if (defined(ENDPOINT6_CONFIG) && NUM_ENDPOINTS >= 6)
	ENDPOINT6_CONFIG,
#elif (NUM_ENDPOINTS >= 6)
	ENDPOINT_UNUSED,
#endif
#if (defined(ENDPOINT7_CONFIG) && NUM_ENDPOINTS >= 7)
	ENDPOINT7_CONFIG,
#elif (NUM_ENDPOINTS >= 7)
	ENDPOINT_UNUSED,
#endif
#if (defined(ENDPOINT8_CONFIG) && NUM_ENDPOINTS >= 8)
	ENDPOINT8_CONFIG,
#elif (NUM_ENDPOINTS >= 8)
	ENDPOINT_UNUSED,
#endif
#if (defined(ENDPOINT9_CONFIG) && NUM_ENDPOINTS >= 9)
	ENDPOINT9_CONFIG,
#elif (NUM_ENDPOINTS >= 9)
	ENDPOINT_UNUSED,
#endif
#if (defined(ENDPOINT10_CONFIG) && NUM_ENDPOINTS >= 10)
	ENDPOINT10_CONFIG,
#elif (NUM_ENDPOINTS >= 10)
	ENDPOINT_UNUSED,
#endif
#if (defined(ENDPOINT11_CONFIG) && NUM_ENDPOINTS >= 11)
	ENDPOINT11_CONFIG,
#elif (NUM_ENDPOINTS >= 11)
	ENDPOINT_UNUSED,
#endif
#if (defined(ENDPOINT12_CONFIG) && NUM_ENDPOINTS >= 12)
	ENDPOINT12_CONFIG,
#elif (NUM_ENDPOINTS >= 12)
	ENDPOINT_UNUSED,
#endif
#if (defined(ENDPOINT13_CONFIG) && NUM_ENDPOINTS >= 13)
	ENDPOINT13_CONFIG,
#elif (NUM_ENDPOINTS >= 13)
	ENDPOINT_UNUSED,
#endif
#if (defined(ENDPOINT14_CONFIG) && NUM_ENDPOINTS >= 14)
	ENDPOINT14_CONFIG,
#elif (NUM_ENDPOINTS >= 14)
	ENDPOINT_UNUSED,
#endif
#if (defined(ENDPOINT15_CONFIG) && NUM_ENDPOINTS >= 15)
	ENDPOINT15_CONFIG,
#elif (NUM_ENDPOINTS >= 15)
	ENDPOINT_UNUSED,
#endif
};


#endif // NUM_ENDPOINTS
#endif // F_CPU >= 20 MHz
