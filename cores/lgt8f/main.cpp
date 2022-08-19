/*
  main.cpp - Main loop for Arduino sketches
  Copyright (c) 2005-2013 Arduino Team.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/
#include <avr/wdt.h>
#include <Arduino.h>

#define OSC_DELAY()	do {\
	_NOP(); _NOP(); _NOP(); _NOP(); _NOP(); _NOP(); _NOP(); _NOP(); _NOP(); _NOP();\
	_NOP(); _NOP(); _NOP(); _NOP(); _NOP(); _NOP(); _NOP(); _NOP(); _NOP(); _NOP();\
} while(0);

// Declared weak in Arduino.h to allow user redefinitions.
int atexit(void (* /*func*/ )()) { return 0; }
uint8_t clock_set = 0;
// Weak empty variant initialization function.
// May be redefined by variant files.
void initVariant() __attribute__((weak));
void initVariant() { }

void setupUSB() __attribute__((weak));
void setupUSB() { }

#if defined(__LGT8FX8E__) || defined(__LGT8FX8P__)
void __patch_wdt(void) \
	     __attribute__((naked)) \
	     __attribute__((section(".init3")));
void __patch_wdt(void)
{
	MCUSR = 0;
	wdt_disable();
}
#endif

void sysClock(uint8_t mode)
{
    if(mode == INT_OSC_32M) {
        // switch to internal crystal
        GPIOR0 = PMCR & 0x9f;
        PMCR = 0x80;
        PMCR = GPIOR0;

        // disable external crystal
        GPIOR0 = PMCR & 0xf3;
        PMCR = 0x80;
        PMCR = GPIOR0;

    }  else if(mode == INT_OSC_32K) {
        // switch to internal 32K crystal
        GPIOR0 = (PMCR & 0x9f) | 0x40;
        PMCR = 0x80;
        PMCR = GPIOR0;

        // disable external crystal
        GPIOR0 = (PMCR & 0xf2) | 0x02;
        PMCR = 0x80;
        PMCR = GPIOR0;
    } else if(mode == EXT_OSC_32K) {
        // enable external 32K OSC crystal
        GPIOR0 = (PMCR & 0xf0) | 0x08;
        PMCR = 0x80;
        PMCR = GPIOR0;
        
        // waiting for crystal stable
        OSC_DELAY();
    
        // switch to external crystal
        GPIOR0 = (PMCR & 0x9f) | 0x60;
        PMCR = 0x80;
        PMCR = GPIOR0;
   } else { // extern OSC

        // set to right prescale first
        CLKPR = 0x80;
        CLKPR = 0x01;

        asm volatile ("nop");
        asm volatile ("nop");

        // enable external 400~32MHz OSC crystal
        GPIOR0 = PMX2 | 0x04;
        PMX2 = 0x80;
        PMX2 = GPIOR0;  //enable extern osc input

        GPIOR0 = (PMCR & 0xf3) | 0x04;
        PMCR = 0x80;
        PMCR = GPIOR0;

        // waiting for crystal stable
        OSC_DELAY();

        // switch to external 400~32MHz crystal
        PMCR = 0x80;
        PMCR = 0xb7;
        OSC_DELAY();
    }
    clock_set = 1;
}

void sysClockPrescale(uint8_t divn)
{
	GPIOR0 = 0x80 | (divn & 0xf);
	CLKPR = 0x80;
	CLKPR = GPIOR0;
}

void sysClockOutput(uint8_t enable)
{
	if (enable)
		CLKPR |= 0x20;  // output cup fre to PB0
	else 
		CLKPR &= ~(0x20);
	//CLKPR |= 0x40;  // output cup fre to PE5
}

void lgt8fx8x_init()
{
#if defined(__LGT8FX8E__)
// store ivref calibration 
	GPIOR1 = VCAL1;
	GPIOR2 = VCAL2;

// enable 1KB E2PROM 
	ECCR = 0x80;
	ECCR = 0x40;

// clock source settings
	if((VDTCR & 0x0C) == 0x0C) {
		// switch to external crystal
		sysClock(EXT_OSC_32M);
	} else {
		CLKPR = 0x80;
		CLKPR = 0x01;
	}
#else

#if defined(__LGT8F_SSOP20__)
        GPIOR0 = PMXCR | 0x07;
        PMXCR = 0x80;
        PMXCR = GPIOR0;
#endif

	// enable 32KRC for WDT
	 GPIOR0 = PMCR | 0x10;
	 PMCR = 0x80;
	 PMCR = GPIOR0;

	 // clock scalar to 16MHz
	 //CLKPR = 0x80;
	 //CLKPR = 0x01;
#endif
}

void lgt8fx8x_clk_src()
{
// select clock source
#if defined(CLOCK_SOURCE)
	sysClock(CLOCK_SOURCE);
#endif

// select clock prescaler
#if defined(F_CPU)
    CLKPR = 0x80;
    #if F_CPU == 32000000L
        #if CLOCK_SOURCE == INT_OSC_32M || CLOCK_SOURCE == EXT_OSC_32M
            CLKPR = SYSCLK_DIV_0;
        #else
            #error "Clock Source Must 32MHz"
        #endif
    #elif F_CPU == 24000000L
		#if CLOCK_SOURCE == EXT_OSC_24M
            CLKPR = SYSCLK_DIV_0;
        #else
            #error "Clock Source Must 24MHz"
        #endif
    #elif F_CPU == 16000000L
        #if CLOCK_SOURCE == INT_OSC_32M || CLOCK_SOURCE == EXT_OSC_32M
            CLKPR = SYSCLK_DIV_2;
        #elif CLOCK_SOURCE == EXT_OSC_16M
            CLKPR = SYSCLK_DIV_0;
        #else
            #error "Clock Source Must 16,32MHZ"
        #endif
    #elif F_CPU == 12000000L
        #if CLOCK_SOURCE == EXT_OSC_24M
            CLKPR = SYSCLK_DIV_2;
        #elif CLOCK_SOURCE == EXT_OSC_12M
            CLKPR = SYSCLK_DIV_0;
        #else
            #error "Clock Source Must 12,24MHZ"
        #endif
    #elif F_CPU == 8000000L
        #if CLOCK_SOURCE == INT_OSC_32M || CLOCK_SOURCE == EXT_OSC_32M
            CLKPR = SYSCLK_DIV_4;
        #elif CLOCK_SOURCE == EXT_OSC_16M
            CLKPR = SYSCLK_DIV_2;
        #elif CLOCK_SOURCE == EXT_OSC_8M
            CLKPR = SYSCLK_DIV_0;        
        #else
            #error "Clock Source Must 8,16,32MHz"
        #endif
    #elif F_CPU == 4000000L
        #if CLOCK_SOURCE == INT_OSC_32M || CLOCK_SOURCE == EXT_OSC_32M
            CLKPR = SYSCLK_DIV_8;
        #elif CLOCK_SOURCE == EXT_OSC_16M
            CLKPR = SYSCLK_DIV_4;
        #elif CLOCK_SOURCE == EXT_OSC_8M
            CLKPR = SYSCLK_DIV_2;
        #elif CLOCK_SOURCE == EXT_OSC_4M
            CLKPR = SYSCLK_DIV_0;
        #else
            #error "Clock Source Must 4,8,16,32MHZ"
        #endif
    #elif F_CPU == 2000000L
        #if CLOCK_SOURCE == INT_OSC_32M || CLOCK_SOURCE == EXT_OSC_32M
            CLKPR = SYSCLK_DIV_16;
        #elif CLOCK_SOURCE == EXT_OSC_16M
            CLKPR = SYSCLK_DIV_8;
        #elif CLOCK_SOURCE == EXT_OSC_8M
            CLKPR = SYSCLK_DIV_4;
        #elif CLOCK_SOURCE == EXT_OSC_4M
            CLKPR = SYSCLK_DIV_2;
        #elif CLOCK_SOURCE == EXT_OSC_2M
            CLKPR = SYSCLK_DIV_0;
        #else
            #error "Clock Source Must 2,8,16,32MHZ"
        #endif

    #elif F_CPU == 1000000L
        #if CLOCK_SOURCE == INT_OSC_32M || CLOCK_SOURCE == EXT_OSC_32M
            CLKPR = SYSCLK_DIV_32;
        #elif CLOCK_SOURCE == EXT_OSC_16M
            CLKPR = SYSCLK_DIV_16;
        #elif CLOCK_SOURCE == EXT_OSC_8M
            CLKPR = SYSCLK_DIV_8;
        #elif CLOCK_SOURCE == EXT_OSC_4M
            CLKPR = SYSCLK_DIV_4;
        #elif CLOCK_SOURCE == EXT_OSC_2M
            CLKPR = SYSCLK_DIV_2;
        #elif CLOCK_SOURCE == EXT_OSC_1M
            CLKPR = SYSCLK_DIV_0;
        #else
            #error "Clock Source Must 1,2,4,8,16,32MHZ"
        #endif
    #elif F_CPU == 400000L
        #if CLOCK_SOURCE == EXT_OSC_400K
            CLKPR = SYSCLK_DIV_0;
        #else
            #error "Clock Source Must 400KHz" 
        #endif
    #elif F_CPU == 32000L
        #if CLOCK_SOURCE == INT_OSC_32K || CLOCK_SOURCE == EXT_OSC_32K
            CLKPR = SYSCLK_DIV_0;
        #else
            #error "Clock Source Must 32KHz"
        #endif
    #endif
	// CLKPR |= 0x20;  // output cup fre to PB0
#endif
}


int main(void)
{

#if defined(__LGT8F__)

	lgt8fx8x_init();

#if defined(CLOCK_SOURCE)
    if (clock_set == 0)
		lgt8fx8x_clk_src();
#endif

#endif	

	init();

	initVariant();

#if defined(USBCON)
	USBDevice.attach();
#endif
	
	setup();
    
	for (;;) {
		loop();
		if (serialEventRun) serialEventRun();
	}
        
	return 0;
}
