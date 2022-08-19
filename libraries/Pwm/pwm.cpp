#include "pwm.h"
#include <avr/wdt.h>
#include <Arduino.h>
#include <wiring_private.h>

void unlockWrite(volatile uint8_t *p, uint8_t val)
{
	uint8_t _o_sreg = SREG;
	volatile uint8_t *cp = p;

	if(p == &PMX1)
		cp = &PMX0;
	cli();
	*cp = 0x80;
	*p = val;
	SREG = _o_sreg;
}

void atomicWriteWord(volatile uint8_t *p, uint16_t val)
{
	uint8_t _o_sreg = SREG;

	cli();
	*(p + 1) = (uint8_t)(val >> 8);
	nop(); nop(); nop();
	*p = (uint8_t)val;
	SREG = _o_sreg;
}

// Log(HSP v3.7): PWM working mode
// Function:
//	wmode: pwm working mode
//		- PWM_MODE_NORMAL: normal single output  //单输出
//		- PWM_MODE_COMPM0: complementary dual output 
//		- PWM_MODE_COMPM1: complementary dual output (inverted)
//		- PWM_MODE_COMPM2: complementary dual output 
//		- PWM_MODE_COMPM3: complementary dual output (inverted)
//	fmode: pwm frequency settings
//		- PWM_FREQ_SLOW: slow range
//		- PWM_FREQ_NORMAL: normal range
//		- PWM_FREQ_FAST: fast range 
//		- PWM_FREQ_BOOST: boost target frequency by x4
//	dband: dead band settings
//		- only valid for complementary working mode 
// note:
//		- Timer 2 is used for system tick, so don't touch!!
//static uint8_t tmr1_boost_en = 0;
//static uint8_t tmr3_boost_en = 0;

void pwmMode(uint8_t pin, uint8_t wmode, uint8_t fmode, uint8_t dband)
{
	volatile uint8_t *pTCCRX = 0;

	uint8_t timer = digitalPinToTimer(pin) & 0xf0;

	if(timer == TIMER0) { // TIMER0
		pTCCRX = &TCCR0B;
		if(wmode == PWM_MODE_NORMAL) {
			cbi(TCCR0B, DTEN0);
			cbi(TCCR0A, COM0B0);
		} else {
			sbi(TCCR0B, DTEN0);
			TCCR0A = (TCCR0A & ~_BV(COM0B0)) | (wmode & 0x10);
			DTR0 = ((dband & 0xf) << 4) | (dband & 0xf);
		}

		if((fmode & PWM_FREQ_BOOST) == PWM_FREQ_BOOST) {
			// enable frequency boost (x4) mode
			sbi(TCKCSR, F2XEN);
			delayMicroseconds(10);
			sbi(TCKCSR, TC2XS0);
		} else if(bit_is_set(TCKCSR, TC2XS0)) {
			cbi(TCKCSR, TC2XS0);
			delayMicroseconds(10);
			cbi(TCKCSR, F2XEN);
		}
	} else if(timer == TIMER1) { // TIMER1
		pTCCRX = &TCCR1B;
		if(wmode == PWM_MODE_NORMAL) {
			cbi(TCCR1C, DTEN1);
			cbi(TCCR1A, COM1B0);
		} else {
			sbi(TCCR1C, DTEN1);
			TCCR1A = (TCCR1A & ~_BV(COM1B0)) | (wmode & 0x10);
			DTR1L = dband;
			DTR1H = dband;
		}
		if((fmode & PWM_FREQ_BOOST) == PWM_FREQ_BOOST) {
			sbi(TCKCSR, F2XEN);
			delayMicroseconds(10);
			sbi(TCKCSR, TC2XS1);
		} else if(bit_is_set(TCKCSR, TC2XS1)) {
			cbi(TCKCSR, TC2XS1);
			delayMicroseconds(10);
			cbi(TCKCSR, F2XEN);
		}		
	} else if(timer == TIMER3) { // TIMER3
		pTCCRX = &TCCR3B;
		if(wmode == PWM_MODE_NORMAL) {
			cbi(TCCR3C, DTEN3);
			cbi(TCCR3A, COM3B0);
		} else {
			sbi(TCCR3C, DTEN3);
			TCCR3A = (TCCR3A & ~_BV(COM3B0)) | (wmode & 0x10);
			DTR3A = dband;
			DTR3B = dband;
		}
	}

	if(pTCCRX == 0) return;

	if((fmode & 0x7f) == PWM_FREQ_SLOW) {
		*pTCCRX = (*pTCCRX & 0xf8) | PWM_FREQ_SLOW;	// prescale = 1024 (slowest mode)
	} else if((fmode & 0x7f) == PWM_FREQ_FAST) {
		*pTCCRX = (*pTCCRX & 0xf8) | PWM_FREQ_FAST; // prescale = 1 (fastest mode)
	} else if ((fmode & 0x7f) == PWM_FREQ_NORMAL) {
		*pTCCRX = (*pTCCRX & 0xf8) | PWM_FREQ_NORMAL;	// prescale = 64 (default)
	}
}

// Log(HSP v3.7): enhanced PWM settings
// Function:
//	- set PWM frequency (unit: Hz), return maximum duty cycle 
// Note: 
//	- only PWM Timer1/Timer3 support frequency update
uint16_t pwmFrequency(uint8_t pin, uint32_t fhz)
{
	uint16_t icrx = 0;
	uint8_t csxs = 0;
	uint8_t boost = 0;
	volatile uint8_t *pICRX = 0;

	uint8_t timer = digitalPinToTimer(pin) & 0xf0;

	// Note for TIMER0 
	// ============================================================================
	// timer 0 working in FPWM mode which TOP is fixed to 0xFF
	// so we can change its prescale to set frequency range (fast/normal/slow)
	// fast mode:	16000000/(1*256) = 62.5K, support boost up to 62.5x4 = 250KHz
	// normal mode:	16000000/(64*256) = 976Hz, support boost up to 3.9KHz
	// slow mode:	16000000/(1024*256) = 61Hz, support boost up to 244Hz
	// ============================================================================

	if(timer == TIMER1) { // TIMER1
		pICRX = &ICR1L;
		csxs = TCCR1B & 0x7;
		boost = bit_is_set(TCKCSR, TC2XF1);
	} else if(timer == TIMER3) { // TIMER3
		pICRX = &ICR3L;
		csxs = TCCR3B & 0x7;
	}

	if(pICRX == 0) return 0xff;

	// DO NOT try to merge the two cases, compiler will try to 
	// optimize the divider if either of oprands is constant value
	if(boost == 0) {
		if(csxs == PWM_FREQ_FAST) { // fast mode
			icrx = (uint16_t) ((F_CPU >> 1) / fhz);
		} else if(csxs == PWM_FREQ_NORMAL) { // normal mode
			icrx = (uint16_t) ((F_CPU >> 7) / fhz);
		} else if(csxs == PWM_FREQ_SLOW) { // slow mode
			icrx = (uint16_t) ((F_CPU >> 11) / fhz);
		}
	} else {
		if(csxs == PWM_FREQ_FAST) { // fast mode
			icrx = (uint16_t) ((64000000UL >> 1) / fhz);
		} else if(csxs == PWM_FREQ_NORMAL) { // normal mode
			icrx = (uint16_t) ((64000000UL >> 7) / fhz);
		} else if(csxs == PWM_FREQ_SLOW) { // slow mode
			icrx = (uint16_t) ((64000000UL >> 11) / fhz);
		}	
	}
	
	atomicWriteWord(pICRX, icrx);

	return icrx;
}

// Log(HSP v3.7):
// Function:
//	- return frequency (in Hz) by give PWM resolution (bits width of duty)
// Note: 
//	- timer0/2 works in FPWM mode, pwm frequency is fixed by given mode
//	- timer1/3 works in PCPWM mode, means frequency reduced by a half
uint32_t pwmResolution(uint8_t pin, uint8_t resBits)
{
	uint8_t csxs = 0;
	uint8_t boost = 0;
	uint32_t freq = 0x0UL;

	uint8_t timer = digitalPinToTimer(pin) & 0xf0;

	if(timer != TIMER1 && timer != TIMER3)
		return 0x0UL;

	if(timer == TIMER1) { // TIMER1
		csxs = TCCR1B & 0x7;
		boost = bit_is_set(TCKCSR, TC2XF1);
	} else if(timer == TIMER3) { // TIMER3
		csxs = TCCR3B & 0x7;
	}	
	
	if(boost != 0) {
		if(csxs == PWM_FREQ_FAST) {
			freq = (64000000UL >> 1) / (1 << resBits);
		} else if(csxs == PWM_FREQ_SLOW) {
			freq = (64000000UL >> 11) / (1 << resBits);
		} else { // PWM_FREQ_NORMAL
			freq = (64000000UL >> 7) / (1 << resBits);
		}
	} else {
		if(csxs == PWM_FREQ_FAST) {
			freq = (F_CPU >> 1) / (1 << resBits);
		} else if(csxs == PWM_FREQ_SLOW) {
			freq = (F_CPU >> 11) / (1 << resBits);
		} else { // PWM_FREQ_NORMAL
			freq = (F_CPU >> 7) / (1 << resBits);
		}
	}

	// update pwm frequency
	pwmFrequency(pin, freq);

	return freq;
}

void pwmWrite(uint8_t pin, uint16_t val)
{
	// We need to make sure the PWM output is enabled for those pins
	// that support it, as we turn it off when digitally reading or
	// writing with them.  Also, make sure the pin is in output mode
	// for consistenty with Wiring, which doesn't require a pinMode
	// call for the analog output pins.

	// duty cycle validation: settings should not overflow of PWM frequency

	switch(digitalPinToTimer(pin)) {
		// XXX fix needed for atmega8
		#if defined(TCCR0) && defined(COM00) && !defined(__AVR_ATmega8__)
		case TIMER0A:
			// connect pwm to pin on timer 0
			sbi(TCCR0, COM00);
			OCR0 = (uint8_t)val; // set pwm duty
			break;
		#endif

		#if defined(TCCR0A) && defined(COM0A1)
		case TIMER0A: // D6
			// connect pwm to pin on timer 0, channel A
			OCR0A = (uint8_t)val; // set pwm duty
			sbi(TCCR0A, COM0A1);
			cbi(TCCR0B, OC0AS);	//*****
			sbi(DDRD, PD6);
			break;
		#if defined(__LGT8FXP48__)
		case TIMER0AX: // E4
			OCR0A = (uint8_t)val;
			sbi(TCCR0A, COM0A1);
			sbi(TCCR0B, OC0AS);	 //*****
			sbi(DDRE, PE4);
			break;
		#endif
		#endif
		#if defined(TCCR0A) && defined(COM0B1)
		case TIMER0B: // D5
			// connect pwm to pin on timer 0, channel B
			OCR0B = (uint8_t)val; // set pwm duty
			sbi(TCCR0A, COM0B1);
			#if defined(__LGT8FXP48__)
			unlockWrite(&PMX0, (PMX0 & ~_BV(C0BF3)));
			#endif
			sbi(DDRD, PD5);
			break;
		#if defined(__LGT8FXP48__)
		case TIMER0BX: // F3
			OCR0B = (uint8_t)val;
			sbi(TCCR0A, COM0B1);
			unlockWrite(&PMX0, (PMX0 | _BV(C0BF3)));
			sbi(DDRF, PF3);
			break;
		#endif
		#endif

		#if defined(TCCR1A) && defined(COM1A1)
		case TIMER1A: // B1
			// connect pwm to pin on timer 1, channel A
			//OCR1A = val; // set pwm duty
			atomicWriteWord(&OCR1AL, val);
			sbi(TCCR1A, COM1A1);
			#if defined(__LGT8FX8P__)
			unlockWrite(&PMX0, (PMX0 & ~_BV(C1AF5)));
			#endif
			sbi(DDRB, PB1);
			break;
		#if defined(__LGT8FX8P__)
			// F5 for LGT8F328P/QFP48
			// E5 for LGT8F328P/QFP32 (tied with F5)
		case TIMER1AX:
		#if defined (__LGT8FX8P32__)
			cbi(DDRE, PE5);
		#endif
			//OCR1A = val;
			atomicWriteWord(&OCR1AL, val);
			sbi(TCCR1A, COM1A1);
			unlockWrite(&PMX0, (PMX0 | _BV(C1AF5)));
			sbi(DDRF, PF5);
		#endif
		#endif

		#if defined(TCCR1A) && defined(COM1B1)
		case TIMER1B: // B2
			// connect pwm to pin on timer 1, channel B
			//OCR1B = val; // set pwm duty
			atomicWriteWord(&OCR1BL, val);
			sbi(TCCR1A, COM1B1);
			#if defined(__LGT8FX8P__)
			unlockWrite(&PMX0, (PMX0 & ~_BV(C1BF4)));
			#endif
			sbi(DDRB, PB2);
			break;

		#if defined(__LGT8FX8P__)
		// F4 for LGT8F328P/QFP48
		// E4 for LGT8F328P/QFP32 (tied with F4)
		case TIMER1BX:
		#if defined(__LGT8FX8P32__)
			cbi(DDRE, PE4);
		#endif
			//OCR1B = val;
			atomicWriteWord(&OCR1BL, val);
			sbi(TCCR1A, COM1B1);
			unlockWrite(&PMX0, (PMX0 | _BV(C1BF4)));
			sbi(DDRF, PF4);
			break;
		#endif
		#endif

		#if defined(TCCR2) && defined(COM21)
		case TIMER2:
			// connect pwm to pin on timer 2
			OCR2 = (uint8_t)val; // set pwm duty
			sbi(TCCR2, COM21);
			break;
		#endif

		#if defined(TCCR2A) && defined(COM2A1)
		case TIMER2A: // B3
			// connect pwm to pin on timer 2, channel A
			OCR2A = (uint8_t)val; // set pwm duty
			sbi(TCCR2A, COM2A1);
			#if defined(__LGT8FX8P48__)
			unlockWrite(&PMX1, (PMX1 & ~_BV(C2AF6)));
			#endif
			sbi(DDRB, PB3);
			break;
		#if defined(__LGT8F8P48__)
		case TIMER2AX: // F6
			OCR2A = (uint8_t)val;
			sbi(TCCR2A, COM2A1);
			unlockWrite(&PMX1, (PMX1 | _BV(C2AF6)));
			sbi(DDRF, PF6);
			break;
		#endif
		#endif

		#if defined(TCCR2A) && defined(COM2B1)
		case TIMER2B: // D3
			// connect pwm to pin on timer 2, channel B
			OCR2B = (uint8_t)val; // set pwm duty
			sbi(TCCR2A, COM2B1);
			#if defined(__LGT8FX8P48__)
			unlockWrite(&PMX1, (PMX1 & ~_BV(C2BF7)));
			#endif
			sbi(DDRD, PD3);
			break;
		#if defined(__LGT8F8P48__)
		case TIMER2BX: // F7
			OCR2B = (uint8_t)val;
			sbi(TCCR2A, COM2B1);
			unlockWrite(&PMX1, (PMX1 | _BV(C2BF7)));
			sbi(DDRF, PF7);
			break;
		#endif
		#endif

		#if defined(TCCR3A) && defined(COM3A1)
		case TIMER3A: // D1 tied with F1
			// connect pwm to pin on timer 3, channel A
			cbi(UCSR0B, TXEN0);
			cbi(DDRD, PD1);
			atomicWriteWord(&OCR3AL, val);
			sbi(TCCR3A, COM3A1);
			sbi(DDRF, PF1);
			break;
		#if defined(__LGT8FX8P48__)
		case TIMER3AX: // F1 standalone
			atomicWriteWord(&OCR3AL, val);
			sbi(TCCR3A, COM3A1);
			unlockWrite(&PMX1, (PMX1 & ~_BV(C3AC)));
			sbi(DDRF, PF1);
			break;
		case TIMER3AA: // OC3A/ACO standalone
			atomicWriteWord(&OCR3AL, val);
			unlockWrite(&PMX1, (PMX1 | _BV(C3AC)));
			sbi(TCCR3A, COM3A1);
			break;
		#endif
		#endif

		#if defined(TCCR3A) && defined(COM3B1)
		case TIMER3BX: // F2 tied with D2
		#if defined(__LGT8FX8P32__)
		case TIMER3B:
			cbi(DDRD, PD2);
		#endif
			// connect pwm to pin on timer 3, channel B
			//OCR3B = val; // set pwm duty
			atomicWriteWord(&OCR3BL, val);
			sbi(TCCR3A, COM3B1);
			sbi(DDRF, PF2);
			break;
		#endif

		#if defined(__LGT8FX8P48__)
		#if defined(TCCR3A) && defined(COM3C1)
		case TIMER3C: // F3
			// connect pwm to pin on timer 3, channel C
			//OCR3C = val; // set pwm duty
			atomicWriteWord(&OCR3CL, val);
			sbi(TCCR3A, COM3C1);
			sbi(DDRF, PF3);
			break;
		#endif
		#endif

		#if defined(TCCR4A)
		case TIMER4A:
			//connect pwm to pin on timer 4, channel A
			sbi(TCCR4A, COM4A1);
			#if defined(COM4A0)// only used on 32U4
			cbi(TCCR4A, COM4A0);
			#endif
			OCR4A = val;// set pwm duty
			break;
		#endif

		#if defined(TCCR4A) && defined(COM4B1)
		case TIMER4B:
			// connect pwm to pin on timer 4, channel B
			sbi(TCCR4A, COM4B1);
			OCR4B = val; // set pwm duty
			break;
		#endif

		#if defined(TCCR4A) && defined(COM4C1)
		case TIMER4C:
			// connect pwm to pin on timer 4, channel C
			sbi(TCCR4A, COM4C1);
			OCR4C = val; // set pwm duty
			break;
		#endif

		#if defined(TCCR4C) && defined(COM4D1)
		case TIMER4D:
			// connect pwm to pin on timer 4, channel D
			sbi(TCCR4C, COM4D1);
			#if defined(COM4D0)// only used on 32U4
			cbi(TCCR4C, COM4D0);
			#endif
			OCR4D = val;// set pwm duty
			break;
		#endif
		#if defined(TCCR5A) && defined(COM5A1)
		case TIMER5A:
			// connect pwm to pin on timer 5, channel A
			sbi(TCCR5A, COM5A1);
			OCR5A = val; // set pwm duty
			break;
		#endif

		#if defined(TCCR5A) && defined(COM5B1)
		case TIMER5B:
			// connect pwm to pin on timer 5, channel B
			sbi(TCCR5A, COM5B1);
			OCR5B = val; // set pwm duty
			break;
		#endif

		#if defined(TCCR5A) && defined(COM5C1)
		case TIMER5C:
			// connect pwm to pin on timer 5, channel C
			sbi(TCCR5A, COM5C1);
			OCR5C = val; // set pwm duty
			break;
		#endif
		#if defined(__LGT8FX8E__) || defined(__LGT8FX8P__)
		case LGTDAO0:
			DAL0 = val;
			break;
		#endif
		#if defined(__LGT8FX8E__)
		case LGTDAO1:
			DAL1 = val;
			break;
		#endif
		case NOT_ON_TIMER:
		default:
			if (val < 128) {
				digitalWrite(pin, LOW);
			} else {
				digitalWrite(pin, HIGH);
			}
	}
}