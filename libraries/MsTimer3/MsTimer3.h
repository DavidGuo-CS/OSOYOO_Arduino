#ifndef MsTimer3_h
#define MsTimer3_h

#ifdef __AVR__
#include <avr/interrupt.h>
#else
#error MsTimer3 library only works on AVR architecture
#endif

#if defined(ARDUINO_AVR_LARDU_328E)
#include "lgtx8p.h"
#endif

namespace MsTimer3 {
    extern unsigned long msecs;
    extern void (*func)();
    extern volatile unsigned long count;
    extern volatile char overflowing;
    extern volatile unsigned int tcnt3;

    void set(unsigned long ms, void (*f)());
    void start();
    void stop();
    void _overflow();
}

#endif
