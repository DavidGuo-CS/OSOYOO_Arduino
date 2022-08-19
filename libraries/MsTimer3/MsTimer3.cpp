#if defined(ARDUINO_AVR_LARDU_328E)
#include <MsTimer3.h>
unsigned long MsTimer3::msecs;
void (*MsTimer3::func)();
volatile unsigned long MsTimer3::count;
volatile char MsTimer3::overflowing;
volatile unsigned int MsTimer3::tcnt3;

void MsTimer3::set(unsigned long ms, void (*f)()) {
    float prescaler = 0.0;
    if (ms == 0)
        msecs = 165;
    else
        msecs = ms*165;    // temp solution
    func = f;

    TIMSK3 = 0;
    TCCR3A &= ~((1<<WGM31) | (1<<WGM30));
    //TCCR3B |= (1<<WGM32);   //CTC模式1
    //TCCR3B &= ~(1<<WGM33);
    TCCR3B &= ~((1<<WGM33) | (1<<WGM32));   //定时器普通模式0
    // OCR3AH = 0;
    // OCR3AL = 0xFF;

    if ((F_CPU >= 1000000UL) && (F_CPU <= 16000000UL)) {   // prescaler set to 64
        TCCR3B &= ~(1<<CS32);
        TCCR3B |= ((1<<CS31) | (1<<CS30));
        //TCCR3B |= (1<<CS32);
        //TCCR3B &= ~((1<<CS31) | (1<<CS30));
        prescaler = 64.0;
    } else if (F_CPU < 1000000UL) {	// prescaler set to 8
        TCCR3B |= (1<<CS31);
        TCCR3B &= ~((1<<CS32) | (1<<CS30));
        prescaler = 8.0;
    } else { // F_CPU > 16Mhz, prescaler set to 256
        TCCR3B |= (1<<CS32);
        TCCR3B &= ~((1<<CS31) | (1<<CS30));
        prescaler = 256.0;
    }
    tcnt3 = 0xFFFF - (unsigned int)((float)F_CPU * 0.001 / prescaler);
}

void MsTimer3::start() {
    count = 0;
    overflowing = 0;
    TCNT3H = (tcnt3 >> 8) & 0xFF;
    TCNT3L = tcnt3 & 0xFF;
    TIMSK3 |= (1<<TOIE3);  //开启溢出中断
    //TIMSK3 |= 1<<OCIE3A;
}

void MsTimer3::stop() {

    TIMSK3 &= ~(1<<TOIE3);
}

void MsTimer3::_overflow() {
    count += 1;

    if (count >= msecs && !overflowing) {
        overflowing = 1;
        count = count - msecs; // subtract ms to catch missed overflows
                                             // set to 0 if you don't want this.
        (*func)();
        overflowing = 0;
    }
}

ISR(__vector_29) {
   if (TIFR3 & (1<<TOV3)) {
        TCNT3H = (MsTimer3::tcnt3 >> 8) & 0xFF;
        TCNT3L = MsTimer3::tcnt3 & 0xFF;
        MsTimer3::_overflow();
    } else if (TIFR3 & (1<<OCF3A)) {
   }
}
#endif
