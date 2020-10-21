// prh - my extremely edited verion of this file
// The app must be compiled MIDI+SERIAL or MIDI4+SERIAL


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

#ifndef _usb_desc_h_
#define _usb_desc_h_

#include <stdint.h>
#include <stddef.h>

#define ENDPOINT_UNUSED			0x00
#define ENDPOINT_TRANSMIT_ONLY		0x15
#define ENDPOINT_RECEIVE_ONLY		0x19
#define ENDPOINT_TRANSMIT_AND_RECEIVE	0x1D
#define ENDPOINT_RECEIVE_ISOCHRONOUS	0x18
#define ENDPOINT_TRANSMIT_ISOCHRONOUS	0x14

// The configuration is MIDI4+SERIAL and the number of cables can vary.

// extern uint16_t MIDI_NUM_CABLES;
// prh - doesnt work

#define VENDOR_ID		        0x16C0
#define PRODUCT_ID		        0x0289  // changed from 489 2020-06-25
#define BCD_DEVICE		        0x0211
#define MANUFACTURER_NAME	    {'h','a','r','m','o','n','i','c','S','u','b','s','y','t','e','m','s'}    // prh changed
#define MANUFACTURER_NAME_LEN	17
#define PRODUCT_NAME		    {'t','e','e','n','s','y','E','x','p','r','e','s','s','i','o','n'}       // prh changed
#define PRODUCT_NAME_LEN	    16
#define EP0_SIZE		        64
#define NUM_ENDPOINTS           5
#define NUM_USB_BUFFERS	        30
#define NUM_INTERFACE		    3
#define CDC_IAD_DESCRIPTOR	    1
#define CDC_STATUS_INTERFACE	0
#define CDC_DATA_INTERFACE	    1	// Serial
#define CDC_ACM_ENDPOINT	    1
#define CDC_RX_ENDPOINT         2
#define CDC_TX_ENDPOINT         3
#define CDC_ACM_SIZE            16
#define CDC_RX_SIZE             64
#define CDC_TX_SIZE             64
#define MIDI_INTERFACE          2	// MIDI
// #define MIDI_NUM_CABLES         2                // prh made into run-time variable
#define MIDI_TX_ENDPOINT        4
#define MIDI_TX_SIZE            64
#define MIDI_RX_ENDPOINT        5
#define MIDI_RX_SIZE            64

#define ENDPOINT1_CONFIG	ENDPOINT_TRANSMIT_ONLY
#define ENDPOINT2_CONFIG	ENDPOINT_RECEIVE_ONLY
#define ENDPOINT3_CONFIG	ENDPOINT_TRANSMIT_ONLY
#define ENDPOINT4_CONFIG	ENDPOINT_TRANSMIT_ONLY
#define ENDPOINT5_CONFIG	ENDPOINT_RECEIVE_ONLY


#ifdef USB_DESC_LIST_DEFINE
#if defined(NUM_ENDPOINTS) && NUM_ENDPOINTS > 0
// NUM_ENDPOINTS = number of non-zero endpoints (0 to 15)
extern const uint8_t usb_endpoint_config_table[NUM_ENDPOINTS];

typedef struct {
	uint16_t	wValue;
	uint16_t	wIndex;
	const uint8_t	*addr;
	uint16_t	length;
} usb_descriptor_list_t;

// prh - removed the const
extern usb_descriptor_list_t usb_descriptor_list[];

#endif // NUM_ENDPOINTS
#endif // USB_DESC_LIST_DEFINE

#endif
