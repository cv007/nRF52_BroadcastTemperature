#pragma once

#include <cstdint>
#include <cstdbool>

#include "nrf_delay.h"

#include "Twim.hpp"
#include "Print.hpp"

#define U32 uint32_t
#define U16 uint16_t
#define I16 int16_t
#define U8 uint8_t
// #define SA [[gnu::always_inline]] static auto
// #define SA [[gnu::noinline]] static auto
#define SA static auto
#define SI static inline
#define SCA static constexpr auto

//untested - sda/scl pins routed wrong on my board :(

/*------------------------------------------------------------------------------
    Si7051 struct

    Si7051<Twi> si7051;

    Si7051 - Silicon Labs temperature IC
------------------------------------------------------------------------------*/
template<typename Twi_>
struct Si7051 {

    //============
        private:
    //============

    SCA Addr_{ 0x40 }; //only one address
    static Twi_& twi_;
    static inline bool isInit_{ false };
    static inline bool isConverting_{ false };

    enum COMMANDS1 { MEASURE_HOLD = 0xE3, MEASURE_NOHOLD = 0xF3, RESET = 0xFE,
                     WRITE_USER = 0xE6, READ_USER = 0xE7 };
    enum COMMANDS2 { READ_ID1 = 0xFA0F, READ_ID2 = 0xFCC9, READ_FIRMREV = 0x84B8 };

SA  readUser    (U8& v)         { U8 tbuf[1]{ READ_USER }; U8 rbuf[1];
                                  if( not twi_.writeRead(tbuf, rbuf) ) return false; 
                                  v = rbuf[0];
                                  return true;
                                }

SA  command     (COMMANDS1 cmd) { U8 tbuf[1]{ cmd }; 
                                  return twi_.write(tbuf);
                                }

    //============
        public:
    //============

                    //bitmasks
    enum RESOLUTION { RES_14BIT, RES_12BIT, RES_13BIT = 0x80, RES_11BIT = 0x81 };


SA  init        ()              { 
                                twi_.init( Addr_, twi_.K400 );
                                //startup time 18-25ms, max 80ms
                                //let caller deal with startup time
                                isInit_ = true;
                                }

SA  deinit      ()              { twi_.deinit(); isInit_ = false; }

SA  reset       ()              { return command( RESET ); } //5-15ms

SA  resolution  (RESOLUTION e)  { 
                                U8 v;
                                if( not readUser(v) ) return false;
                                v and_eq compl 0x81; //clear res bits
                                U8 tbuf[2]{ WRITE_USER, v bitor e };
                                return twi_.write(tbuf);
                                }

SA  isPowerOk   ()              { 
                                U8 v;
                                if( not readUser(v) ) return false;
                                return v bitand 0x40; 
                                }

                                //blocking on clock stretch, up to 10.8ms
SA  tempWait    (U16& v)        { 
                                U8 tbuf[1]{ MEASURE_HOLD }; U8 rbuf[2]; 
                                if( not twi_.writeRead(tbuf, rbuf) ) return false;
                                v = (rbuf[0]<<8) bitor rbuf[1];
                                return true; 
                                }

                                //start measurement
SA  tempStart   ()              {   
                                if( not command(MEASURE_NOHOLD) ) return false;
                                isConverting_ = true;
                                return true; //started
                                }

                                //poll for temp (after tempStart)
SA  tempPoll    (U16& v)        { 
                                if( not isConverting_ ) return false;
                                U8 rbuf[2];
                                if( not twi_.read(rbuf) ) return false; //not ready (nack)
                                v = (rbuf[0]<<8) bitor rbuf[1];
                                isConverting_ = false;
                                return true; //you now have a value
                                }

                                //serial number, not checking crc values
SA  esn         (U8 (&buf)[8])  {
                                U8 tbuf[2]{ READ_ID1>>8, READ_ID1&0xff };
                                U8 rbuf[8];
                                if( not twi_.writeRead(tbuf, rbuf) ) return false;
                                buf[0] = rbuf[0]; buf[1] = rbuf[2]; buf[2] = rbuf[4]; buf[3] = rbuf[6];
                                tbuf[0] = READ_ID2>>8; tbuf[1] = READ_ID2;
                                if( not twi_.writeRead(tbuf, rbuf) ) return false;
                                buf[4] = rbuf[0]; buf[5] = rbuf[2]; buf[6] = rbuf[4]; buf[7] = rbuf[6];
                                return true;
                                }

                                //firmware revision (0xFF = 1.0, 0x20 = 2.0)
SA  firmware    (U8& v)         {
                                U8 tbuf[2]{ READ_FIRMREV>>8, READ_FIRMREV&0xff };
                                U8 rbuf[1];
                                if( not twi_.writeRead(tbuf, rbuf) ) return false;
                                v = rbuf[0];
                                return true;
                                }

/*
temp code = XXXXXXXX XXXXXX00
temp C = (175.72 * temp code / 65536) - 46.85
26796 = 25C = 77C
*/

SA  x100C   (U16 v) -> I16      { return ((v * 17572L)>>16) - 4685; }
SA  x10C    (U16 v) -> I16      { return x100C(v) / 10; }
SA  x100F   (U16 v) -> I16      { return x100C(v) * 9L / 5 + 3200; }
SA  x10F    (U16 v) -> I16      { return x100F(v) / 10; }

};



#undef U32 
#undef U16 
#undef I16 
#undef U8 
#undef SA 
#undef SI
#undef SCA 

/*

Si7051

1.9v - 3.6v
-40C to +125C

Tconv 14bit max 10.8ms
      13bit      6.2ms
      12bit      3.8ms
      11bit      2.4ms

Idd conversion in progress - 90-120ua
    standby range min-max 0.06ua - 3.6ua
    powerup peak 4ma

powerup time 25C - 18-25ms
             max - 80ms
soft reset time - 5-15ms

SCL max 400kHz

address - fixed 0x40

commands-
    Measure Temperature, Hold Master Mode       0xE3
    Measure Temperature, No Hold Master Mode    0xF3
    Reset                                       0xFE
    Write User Register 1                       0xE6
    Read User Register 1                        0xE7
    Read Electronic ID 1st Byte                 0xFA 0x0F
    Read Electronic ID 2nd Byte                 0xFC 0xC9
    Read Firmware Revision                      0x84 0xB8

*/