#pragma once
#define F_CPU 32000000UL

// includes
#include <avr/interrupt.h>
#include <util/delay.h>
#include "Descriptors.h"
#include "usb/usb.h"
#include <avr/eeprom.h>
#include "usb_pipe.h"
#include <avr/io.h>

bool timeout_or_sampling_no_longer_enabled = 0;

USB_PIPE(ep_in, 0x81 | USB_EP_PP, USB_EP_TYPE_BULK_gc, 64, 512, 1, 0, PIPE_ENABLE_FLUSH);

// Queue a byte to be sent over the bulk EP. Blocks if the buffer is full
static inline void send_byte(uint8_t byte){
    // this should never actually block if your buffer is big enough
    while (!usb_pipe_can_write(&ep_in, 1));
    pipe_write_byte(ep_in.pipe, byte);
    USB.INTFLAGSBSET = USB_TRNIF_bm;
}

// Sends a break to end the USB read and flushes the USB pipe
static inline void break_and_flush(){
    usb_pipe_flush(&ep_in);
    USB.INTFLAGSBSET = USB_TRNIF_bm;
    while (!usb_pipe_can_write(&ep_in, 1)){
        if (timeout_or_sampling_no_longer_enabled){
            usb_pipe_reset(&ep_in);
            return;
        }
    }
} 



void EVENT_USB_Device_ConfigurationChanged(uint8_t config){
	PORTR.OUTSET = 1 << 1;	
	usb_pipe_init(&ep_in);
}

ISR(USB_BUSEVENT_vect){
	if (USB.INTFLAGSACLR & USB_SOFIF_bm){
		USB.INTFLAGSACLR = USB_SOFIF_bm;
	}else if (USB.INTFLAGSACLR & (USB_CRCIF_bm | USB_UNFIF_bm | USB_OVFIF_bm)){
		USB.INTFLAGSACLR = (USB_CRCIF_bm | USB_UNFIF_bm | USB_OVFIF_bm);
	}else if (USB.INTFLAGSACLR & USB_STALLIF_bm){
		USB.INTFLAGSACLR = USB_STALLIF_bm;
	}else{
		USB.INTFLAGSACLR = USB_SUSPENDIF_bm | USB_RESUMEIF_bm | USB_RSTIF_bm;
		USB_Evt_Task();
	}
}

ISR(USB_TRNCOMPL_vect){
	USB.FIFOWP = 0;
	USB.INTFLAGSBCLR = USB_SETUPIF_bm | USB_TRNIF_bm;
	usb_pipe_handle(&ep_in);
	USB_Task();
}

uint8_t bitmap[8];
uint8_t sensorData[256];
uint8_t calibrationData[512];
