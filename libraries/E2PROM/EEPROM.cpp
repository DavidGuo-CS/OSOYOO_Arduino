#include "Arduino.h"
#include "EEPROM.h"

int eeprom_size(bool theoretical)
{
    if ( ECCR & 0x40 ) // EEPROM emulation enabled ?
    {
        return theoretical ?
            (1024 << (ECCR & 0x3)): // thoerical size of the emulated EEPROM (see notes above)
            (EEROM_1KB_PAGE_FREE_SIZE << (ECCR & 0x3)); // actual number of bytes available to the user (see notes above)
    }
    return 0;
}

uint16_t eeprom_continuous_address(uint16_t address)
{
    // we recalculate the address so that we automatically skip 
    // every reserved last cell of every 1KB page (see notes above)
    if ( address >= EEROM_1KB_PAGE_FREE_SIZE )
    {
        address = (1024 * (address / EEROM_1KB_PAGE_FREE_SIZE)) // selects the approriate 1KB page 
            + (address % EEROM_1KB_PAGE_FREE_SIZE); // selects the approriate byte on this 1KB page
    }
    return address;
}

uint8_t eeprom_read_byte(uint16_t address)
{
    address = eeprom_continuous_address(address);
    if (address >= (uint16_t)eeprom_size(false)) return 0;

    EEARL = address & 0xff;
    EEARH = (address >> 8); 
     
    EECR |= (1 << EERE);
    __asm__ __volatile__ ("nop" ::);
    __asm__ __volatile__ ("nop" ::);

    return EEDR;
}

void eeprom_write_byte(uint16_t address, uint8_t value)
{
    address = eeprom_continuous_address(address);
    if (address >= (uint16_t)eeprom_size(false)) return;

    uint8_t	__bk_sreg = SREG;
    // set address & data
    EEARL = address & 0xff;
    EEARH = (address >> 8);
    EEDR = value;
    cli();
    EECR = 0x04;
    EECR = 0x02;
    SREG = __bk_sreg;
}

#if defined(__LGT8FX8P__) 
void lgt_eeprom_read_block( uint8_t *pbuf, uint16_t address, uint16_t len)
{
    uint16_t i;

    uint8_t *p = pbuf;

    for (i = 0; i < len; i++) {
        *p++ = eeprom_read_byte(address+i);
    }
}

void lgt_eeprom_write_block( uint8_t *pbuf, uint16_t address, uint16_t len)
{
    uint16_t i;

    uint8_t *p = pbuf;

    for(i = 0; i < len; i++) {
        eeprom_write_byte( address+i, *p++);
    }
}

uint32_t eeprom_read32(uint16_t address)
{
    uint32_t dwTmp;

    EEARL = address & 0xff;
    EEARH = (address >> 8);

    EECR |= (1 << EERE);

    __asm__ __volatile__ ("nop" ::);
    __asm__ __volatile__ ("nop" ::);

    dwTmp = E2PD0;
    dwTmp |= ((uint32_t)E2PD1 << 8);
    dwTmp |= ((uint32_t)E2PD2 << 16);
    dwTmp |= ((uint32_t)E2PD3 << 24);

    // return data from data register
    return dwTmp;
}

void eeprom_write32(uint16_t address, uint32_t value)
{
    uint8_t __bk_sreg = SREG;

    EEARL = 0;
    EEDR = value;
    EEARL = 1;
    EEDR = value >> 8;
    EEARL = 2;
    EEDR = value >> 16;
    EEARL = 3;
    EEDR = value >> 24;

    EEARH = address >> 8;
    EEARL = address & 0xff;

    // Program Mode
    cli();
    EECR = 0x44;
    EECR = 0x42;

    SREG = __bk_sreg;
}

// ----------------------------------------------------------------------
// public: write bundle of data to E2PROM with SWM mode enable
// ----------------------------------------------------------------------
void eeprom_write_swm(uint16_t address, uint32_t *pData, uint16_t length)
{
    uint16_t i;
    uint8_t __bk_sreg;

    eeprom_reset();
    eeprom_SWM_ON();

    EEARH = address >> 8;
    EEARL = address;

    for( i = 0; i < length; i++ ) 
    {
        if( i == (length - 1) )  // the last word
        {
            eeprom_SWM_OFF();
        }

        E2PD0 = (uint8_t) pData[i];
        E2PD1 = (uint8_t)(pData[i] >> 8);
        E2PD2 = (uint8_t)(pData[i] >> 16);
        E2PD3 = (uint8_t)(pData[i] >> 24);

        __bk_sreg = SREG;
        cli();

        EECR = 0x44;
        EECR = 0x42;
        SREG = __bk_sreg;
    }
}

// ----------------------------------------------------------------------
// public: read bundle of data to E2PROM with SWM mode enable
// ----------------------------------------------------------------------
void eeprom_read_swm(uint16_t address, uint32_t *pData, uint16_t length)
{
    uint16_t i;

    eeprom_reset();
    eeprom_SWM_ON();

    EEARH = address >> 8;
    EEARL = address;

    for(i = 0; i < length; i++) 
    {
        if(i == (length - 1)) // the last word
        {
            eeprom_SWM_OFF();
        }

        EECR |= (1 << EERE);
        
        __asm__ __volatile__ ("nop" ::);
        __asm__ __volatile__ ("nop" ::);

        pData[i]  =  (uint32_t)E2PD0;
        pData[i] |= ((uint32_t)E2PD1 << 8);
        pData[i] |= ((uint32_t)E2PD2 << 16);
        pData[i] |= ((uint32_t)E2PD3 << 24);
    }
}
#endif

