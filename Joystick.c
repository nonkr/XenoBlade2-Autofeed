/*
Nintendo Switch Fightstick - Proof-of-Concept

Based on the LUFA library's Low-Level Joystick Demo
	(C) Dean Camera
Based on the HORI's Pokken Tournament Pro Pad design
	(C) HORI

This project implements a modified version of HORI's Pokken Tournament Pro Pad
USB descriptors to allow for the creation of custom controllers for the
Nintendo Switch. This also works to a limited degree on the PS3.

Since System Update v3.0.0, the Nintendo Switch recognizes the Pokken
Tournament Pro Pad as a Pro Controller. Physical design limitations prevent
the Pokken Controller from functioning at the same level as the Pro
Controller. However, by default most of the descriptors are there, with the
exception of Home and Capture. Descriptor modification allows us to unlock
these buttons for our use.
*/

#include "Joystick.h"
#define ALERT_WHEN_DONE

typedef enum {
	UP,
	DOWN,
	LEFT,
	RIGHT,
	X,
	Y,
	A,
	B,
	L,
	R,
	PLUS,
	NOTHING,
	TRIGGERS
} Buttons_t;

typedef struct {
	Buttons_t button;
	uint16_t duration;
} command; 

static int feed_time = 10; // if you don't upgrade your package capacity

static const command sync_controller[] = {
	{ NOTHING,  250 },
	{ TRIGGERS,   5 },
	{ NOTHING,  150 },
	{ TRIGGERS,   5 },
	{ NOTHING,  150 },
	{ A,          5 },
	{ NOTHING,  250 }
};

static const command buy_item[] = {
    { A,          5 },
    { NOTHING,  300 },
	{ A,          5 }, 
	{ NOTHING,  300 },
	{ A,          5 }, 
	{ NOTHING,  300 }, 
	{ A,          5 }, 
	{ NOTHING,  300 },
	{ A,          5 },
	{ NOTHING,  300 },
	{ LEFT,       5 },
	{ NOTHING,  300 },
	{ A,          5 },
	{ NOTHING,  300 }, 
	{ B,          5 },
	{ NOTHING,  300 },
	{ B,          5 },
	{ NOTHING,  300 },
    { PLUS,       5 },
	{ NOTHING,  300 },
	{ A,          5 },
	{ NOTHING,  300 },
	{ A,          5 },
	{ NOTHING,  300 },
    { LEFT,       5 },
	{ NOTHING,  300 },
	{ A,          5 },
	{ NOTHING,  300 }
};

static const command feed_item[] = {
	{ A,	      5 },
	{ NOTHING,   300 },
	{ A,          5 },
	{ NOTHING,   300 },
	{ A,          5 },
	{ NOTHING,   300 }
};

static const command after_feed[] = {
	{ B,          5 },
	{ NOTHING,   300 },
	{ B,          5 },
	{ NOTHING,   300 },
	{ B,          5 },
	{ NOTHING,   300 },
	{ B,          5 },
	{ NOTHING,   300 }
};

// Main entry point.
int main(void) {
	// We'll start by performing hardware and peripheral setup.
	SetupHardware();
	// We'll then enable global interrupts for our use.
	GlobalInterruptEnable();
	// Once that's done, we'll enter an infinite loop.
	for (;;)
	{
		// We need to run our task to process and deliver data for our IN and OUT endpoints.
		HID_Task();
		// We also need to run the main USB management task.
		USB_USBTask();
	}
}

// Configures hardware and peripherals, such as the USB peripherals.
void SetupHardware(void) {
	// We need to disable watchdog if enabled by bootloader/fuses.
	MCUSR &= ~(1 << WDRF);
	wdt_disable();

	// We need to disable clock division before initializing the USB hardware.
	clock_prescale_set(clock_div_1);
	// We can then initialize our hardware and peripherals, including the USB stack.

	#ifdef ALERT_WHEN_DONE
	// Both PORTD and PORTB will be used for the optional LED flashing and buzzer.
	#warning LED and Buzzer functionality enabled. All pins on both PORTB and \
PORTD will toggle when printing is done.
	DDRD  = 0xFF; //Teensy uses PORTD
	PORTD =  0x0;
                  //We'll just flash all pins on both ports since the UNO R3
	DDRB  = 0xFF; //uses PORTB. Micro can use either or, but both give us 2 LEDs
	PORTB =  0x0; //The ATmega328P on the UNO will be resetting, so unplug it?
	#endif
	// The USB stack should be initialized last.
	USB_Init();
}

// Fired to indicate that the device is enumerating.
void EVENT_USB_Device_Connect(void) {
	// We can indicate that we're enumerating here (via status LEDs, sound, etc.).
}

// Fired to indicate that the device is no longer connected to a host.
void EVENT_USB_Device_Disconnect(void) {
	// We can indicate that our device is not ready (via status LEDs, sound, etc.).
}

// Fired when the host set the current configuration of the USB device after enumeration.
void EVENT_USB_Device_ConfigurationChanged(void) {
	bool ConfigSuccess = true;

	// We setup the HID report endpoints.
	ConfigSuccess &= Endpoint_ConfigureEndpoint(JOYSTICK_OUT_EPADDR, EP_TYPE_INTERRUPT, JOYSTICK_EPSIZE, 1);
	ConfigSuccess &= Endpoint_ConfigureEndpoint(JOYSTICK_IN_EPADDR, EP_TYPE_INTERRUPT, JOYSTICK_EPSIZE, 1);

	// We can read ConfigSuccess to indicate a success or failure at this point.
}

// Process control requests sent to the device from the USB host.
void EVENT_USB_Device_ControlRequest(void) {
	// We can handle two control requests: a GetReport and a SetReport.

	// Not used here, it looks like we don't receive control request from the Switch.
}

// Process and deliver data from IN and OUT endpoints.
void HID_Task(void) {
	// If the device isn't connected and properly configured, we can't do anything here.
	if (USB_DeviceState != DEVICE_STATE_Configured)
		return;

	// We'll start with the OUT endpoint.
	Endpoint_SelectEndpoint(JOYSTICK_OUT_EPADDR);
	// We'll check to see if we received something on the OUT endpoint.
	if (Endpoint_IsOUTReceived())
	{
		// If we did, and the packet has data, we'll react to it.
		if (Endpoint_IsReadWriteAllowed())
		{
			// We'll create a place to store our data received from the host.
			USB_JoystickReport_Output_t JoystickOutputData;
			// We'll then take in that data, setting it up in our storage.
			while(Endpoint_Read_Stream_LE(&JoystickOutputData, sizeof(JoystickOutputData), NULL) != ENDPOINT_RWSTREAM_NoError);
			// At this point, we can react to this data.

			// However, since we're not doing anything with this data, we abandon it.
		}
		// Regardless of whether we reacted to the data, we acknowledge an OUT packet on this endpoint.
		Endpoint_ClearOUT();
	}

	// We'll then move on to the IN endpoint.
	Endpoint_SelectEndpoint(JOYSTICK_IN_EPADDR);
	// We first check to see if the host is ready to accept data.
	if (Endpoint_IsINReady())
	{
		// We'll create an empty report.
		USB_JoystickReport_Input_t JoystickInputData;
		// We'll then populate this report with what we want to send to the host.
		GetNextReport(&JoystickInputData);
		// Once populated, we can output this data to the host. We do this by first writing the data to the control stream.
		while(Endpoint_Write_Stream_LE(&JoystickInputData, sizeof(JoystickInputData), NULL) != ENDPOINT_RWSTREAM_NoError);
		// We then send an IN packet on this endpoint.
		Endpoint_ClearIN();
	}
}

typedef enum {
	SYNC_CONTROLLER,
	BUY_ITEM,
	FEED_ITEM,
	AFTER_FEED
} State_t;
State_t state = SYNC_CONTROLLER;


USB_JoystickReport_Input_t last_report;

int report_count = 0;
int bufindex = 0;
int duration_count = 0;
int portsval = 0;

// Prepare the next report for the host.
void GetNextReport(USB_JoystickReport_Input_t* const ReportData) {

	// Prepare an empty report
	memset(ReportData, 0, sizeof(USB_JoystickReport_Input_t));
	ReportData->LX = STICK_CENTER;
	ReportData->LY = STICK_CENTER;
	ReportData->RX = STICK_CENTER;
	ReportData->RY = STICK_CENTER;
	ReportData->HAT = HAT_CENTER;

	// States and moves management
	switch (state)
	{
        
    case SYNC_CONTROLLER:

        switch (sync_controller[bufindex].button)
        {
        case TRIGGERS:
            ReportData->Button |= SWITCH_L | SWITCH_R;
            break;
        case A:
            ReportData->Button |= SWITCH_A;
			break;
        case NOTHING:
            ReportData->LX = STICK_CENTER;
            ReportData->LY = STICK_CENTER;
            ReportData->RX = STICK_CENTER;
            ReportData->RY = STICK_CENTER;
            ReportData->HAT = HAT_CENTER;
            break;
		}
        duration_count++;
        
        if (duration_count > sync_controller[bufindex].duration)
        {
            bufindex++;
            duration_count = 0;				
        }
        if (bufindex > (int) (sizeof(sync_controller)/sizeof(sync_controller[0]) - 1)){
            portsval = ~portsval;
            PORTD = portsval; //flash LED(s) and sound buzzer if attached
            PORTB = portsval;
            state = BUY_ITEM;
            bufindex = 0;
        }
        break;
        
    case BUY_ITEM:
        
	   	switch (buy_item[bufindex].button)
        {
        case LEFT:
            ReportData->HAT = HAT_LEFT;				
            break;

        case A:
            ReportData->Button |= SWITCH_A;
            break;
            
        case B:
            ReportData->Button |= SWITCH_B;
            break;

        case PLUS:
            ReportData->Button |= SWITCH_PLUS;
            break;
            
        case NOTHING:
            ReportData->LX = STICK_CENTER;
            ReportData->LY = STICK_CENTER;
            ReportData->RX = STICK_CENTER;
            ReportData->RY = STICK_CENTER;
            ReportData->HAT = HAT_CENTER;
            break;
        }

        duration_count++;
        
        if (duration_count > buy_item[bufindex].duration)
        {
            bufindex++;
            duration_count = 0;				
        }
        
        
        if (bufindex > (int)( sizeof(buy_item) / sizeof(buy_item[0])) - 1)
        {
            portsval = ~portsval;
            PORTD = portsval; //flash LED(s) and sound buzzer if attached
            PORTB = portsval;
            state = FEED_ITEM;
            bufindex = 0;
        }
        
        break;

    case FEED_ITEM:
        
        switch(feed_item[bufindex].button){
            
        case A:
            ReportData->Button |= SWITCH_A;
            break;
            
        case NOTHING:
            ReportData->LX = STICK_CENTER;
                ReportData->LY = STICK_CENTER;
                ReportData->RX = STICK_CENTER;
                ReportData->RY = STICK_CENTER;
                ReportData->HAT = HAT_CENTER;
                break;
        }
        duration_count++;
        
        if (duration_count > feed_item[bufindex].duration)
        {
            bufindex++;
            duration_count = 0;				
            }
        
        
        if (bufindex > (int)( sizeof(feed_item) / sizeof(feed_item[0])) - 1)
        {
            bufindex = 0;
            feed_time-- ;
            if (feed_time < 1){
                portsval = ~portsval;
                PORTD = portsval; //flash LED(s) and sound buzzer if attached
                PORTB = portsval;
                state = AFTER_FEED;
                feed_time = 10;
            }
        }
        
        break;
        
    case AFTER_FEED:
         switch(after_feed[bufindex].button){

            case B:
                ReportData->Button |= SWITCH_B;
            break;

            case NOTHING:
                ReportData->LX = STICK_CENTER;
                ReportData->LY = STICK_CENTER;
                ReportData->RX = STICK_CENTER;
                ReportData->RY = STICK_CENTER;
                ReportData->HAT = HAT_CENTER;
            break;
            }
            duration_count++;

            if (duration_count > after_feed[bufindex].duration)
            {
                bufindex++;
                duration_count = 0;				
            }

            if (bufindex > (int) (sizeof(after_feed) / sizeof(after_feed[0]) - 1))
            {
                bufindex = 0;
                portsval = ~portsval;
                PORTD = portsval; //flash LED(s) and sound buzzer if attached
                PORTB = portsval;
                state = BUY_ITEM;
            }

            break;
    }
    return;
            
            
                
/*
		case DONE:
			#ifdef ALERT_WHEN_DONE
			portsval = ~portsval;
			PORTD = portsval; //flash LED(s) and sound buzzer if attached
			PORTB = portsval;
			_delay_ms(250);
			#endif
			return;
	}
*/

	// // Inking
	// if (state != SYNC_CONTROLLER && state != SYNC_POSITION)
	// 	if (pgm_read_byte(&(image_data[(xpos / 8) + (ypos * 40)])) & 1 << (xpos % 8))
	// 		ReportData->Button |= SWITCH_A;

	// Prepare to echo this report

}
