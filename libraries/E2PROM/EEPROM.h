/*
  EEPROM.h - EEPROM library
  Original Copyright (c) 2006 David A. Mellis.  All right reserved.
  New version by Christopher Andrews 2015.
  Modifications from https://github.com/LGTMCU/Larduino_HSP
  New Modifications 20201106 by https://github.com/SuperUserNameMan/ :
	- add support for standard Arduino EEPROM API
	- add C-like lgt_eeprom_xxxx() API
	- put everything into a single EEPROM.h header file
	- old projects can keep using the original LGT EEPROM API by defining USE_LGT_EEPROM_API
	- add comments documenting how EEPROM emulation works with LGT8F328p

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

#ifndef EEPROM_h
#define EEPROM_h

#include <inttypes.h>
#include <Arduino.h>

#ifndef EEROM_SIZE
#define EEROM_SIZE 1
#endif

#define	eeprom_SWM_ON()   do { ECCR = 0x80; ECCR |= 0x10; } while(0);
#define	eeprom_SWM_OFF()  do { ECCR = 0x80; ECCR &= 0xEF; } while(0);
#define	eeprom_reset()    do { ECCR |= 0x20; } while(0)

#define EEROM_1KB_PAGE_FREE_SIZE (uint16_t)(1024-2)

void eeprom_init(uint8_t _eerom_size = 1);
int eeprom_size(bool theoretical = false);
uint16_t eeprom_continuous_address(uint16_t address);
uint8_t eeprom_read_byte(uint16_t address);
void eeprom_write_byte(uint16_t address, uint8_t value);

void eeprom_read_block(uint8_t *pbuf, uint16_t address, uint16_t len);
void eeprom_write_block(uint8_t *pbuf, uint16_t address, uint16_t len);

// ----------------------------------------------------------------------
// read/write native 32 bits data from/to E2PROM
// ----------------------------------------------------------------------
uint32_t eeprom_read32(uint16_t address );
void eeprom_write32(uint16_t address, uint32_t value);

// ----------------------------------------------------------------------
// read/write bundle of data from/to E2PROM with SWM mode enable
// ----------------------------------------------------------------------
void eeprom_write_swm(uint16_t address, uint32_t *pData, uint16_t length);
void eeprom_read_swm(uint16_t address, uint32_t *pData, uint16_t length);

struct EERef
{

    EERef( const int index )
        : index( index )                 {}

    //Access/read members.
    uint8_t operator*() const            { return eeprom_read_byte(index); }
    operator uint8_t() const             { return **this; }

    //Assignment/write members.
    EERef &operator=( const EERef &ref ) { return *this = *ref; }
    EERef &operator=( uint8_t in )       { return eeprom_write_byte( index, in), *this;  }
    EERef &operator +=( uint8_t in )     { return *this = **this + in; }
    EERef &operator -=( uint8_t in )     { return *this = **this - in; }
    EERef &operator *=( uint8_t in )     { return *this = **this * in; }
    EERef &operator /=( uint8_t in )     { return *this = **this / in; }
    EERef &operator ^=( uint8_t in )     { return *this = **this ^ in; }
    EERef &operator %=( uint8_t in )     { return *this = **this % in; }
    EERef &operator &=( uint8_t in )     { return *this = **this & in; }
    EERef &operator |=( uint8_t in )     { return *this = **this | in; }
    EERef &operator <<=( uint8_t in )    { return *this = **this << in; }
    EERef &operator >>=( uint8_t in )    { return *this = **this >> in; }

    EERef &update( uint8_t in )          { return  in != *this ? *this = in : *this; }

    /** Prefix increment/decrement **/
    EERef& operator++()                  { return *this += 1; }
    EERef& operator--()                  { return *this -= 1; }

    /** Postfix increment/decrement **/
    uint8_t operator++ (int)
    { 
        uint8_t ret = **this;
        return ++(*this), ret;
    }

    uint8_t operator-- (int)
    { 
        uint8_t ret = **this;
        return --(*this), ret;
    }

    int index; //Index of current EEPROM cell.
};

/***
EEPtr class.

This object is a bidirectional pointer to EEPROM cells represented by EERef objects.
Just like a normal pointer type, this can be dereferenced and repositioned using 
increment/decrement operators.
***/

struct EEPtr
{
    EEPtr( const int index )
        : index( index )                {}
        
    operator int() const                { return index; }
    EEPtr &operator=( int in )          { return index = in, *this; }

    //Iterator functionality.
    bool operator!=( const EEPtr &ptr ) { return index != ptr.index; }
    EERef operator*()                   { return index; }

    /** Prefix & Postfix increment/decrement **/
    EEPtr& operator++()                 { return ++index, *this; }
    EEPtr& operator--()                 { return --index, *this; }
    EEPtr operator++ (int)              { return index++; }
    EEPtr operator-- (int)              { return index--; }

    int index; //Index of current EEPROM cell.
    };

/***
	EEPROMClass class.
	
	This object represents the entire EEPROM space.
	It wraps the functionality of EEPtr and EERef into a basic interface.
	This class is also 100% backwards compatible with earlier Arduino core releases.
***/

struct EEPROMClass
{
    EEPROMClass() {
#if EEROM_SIZE == 0
        // disable E2PROM
        ECCR = 0x80;
        ECCR = 0x00;
#elif EEROM_SIZE == 2
        // enable 2KB E2PROM
        ECCR = 0x80;
        ECCR = 0x4C | 0x01;
#elif EEROM_SIZE == 4
        // enable 4KB E2PROM
        ECCR = 0x80;
        ECCR = 0x4C | 0x02;
#elif EEROM_SIZE == 8
        // enable 8KB E2PROM
        ECCR = 0x80;
        ECCR = 0x4C | 0x03;
#else
        ECCR = 0x80;
        ECCR = 0x4C;
#endif
    };

    //Basic user access methods.
    EERef operator[]( const int idx )    { return idx; }
    uint8_t read( int idx )              { return EERef( idx ); }
    void write( int idx, uint8_t val )   { (EERef( idx )) = val; }
    void update( int idx, uint8_t val )  { EERef( idx ).update( val ); }

    //STL and C++11 iteration capability.
    EEPtr begin()                        { return 0x00; }
    EEPtr end()                          { return length(); } //Standards requires this to be the item after the last valid entry. The returned pointer is invalid.
    uint16_t length()                    { return (eeprom_size(false)); }

    //Functionality to 'get' and 'put' objects to and from EEPROM.
    template< typename T > T &get( int idx, T &t )
    {
        EEPtr e = idx;
        uint8_t *ptr = (uint8_t*) &t;
        for( int count = sizeof(T) ; count ; --count, ++e )  *ptr++ = *e;
        return t;
    }

    template< typename T > const T &put( int idx, const T &t )
    {
        EEPtr e = idx;
        const uint8_t *ptr = (const uint8_t*) &t;
        for( int count = sizeof(T) ; count ; --count, ++e )  (*e).update( *ptr++ );
        return t;
    }
};

static EEPROMClass EEPROM;

#endif

