#ifndef PWM_H_
#define PWM_H_
#include <stdint.h>

#ifdef __cplusplus
extern "C"{
#endif
// PWM workong mode and frequency settings
uint16_t pwmFrequency(uint8_t, uint32_t);
uint32_t pwmResolution(uint8_t, uint8_t);

#define PWM_MODE_NORMAL   0x80
#define PWM_MODE_COMPM0   0x00
#define PWM_MODE_COMPM1   0x10
#define PWM_MODE_COMPM2   0x10
#define PWM_MODE_COMPM3   0x10

#define PWM_MODE_SOLO     0x80
#define PWM_MODE_DUO0     0x00
#define PWM_MODE_DUO1     0x10
#define PWM_MODE_DUO2     0x10
#define PWM_MODE_DUO3     0x10

#define PWM_FREQ_BOOST    0x80
#define PWM_FREQ_FAST     0x01
#define PWM_FREQ_NORMAL   0x03
#define PWM_FREQ_SLOW     0x05
void pwmMode(uint8_t pin, uint8_t wmode, uint8_t fmode = PWM_FREQ_FAST, uint8_t dband = 0);
void pwmWrite(uint8_t, uint16_t);
#ifdef __cplusplus
} // extern "C"
#endif

#endif