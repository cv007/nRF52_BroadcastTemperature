#pragma once

#include <cstdint>
#include <cstdbool>

#include "nrf_delay.h"

#include "Gpio.hpp"
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

/*------------------------------------------------------------------------------
    TMP117 - Texas Instruments temperature IC
------------------------------------------------------------------------------*/


/*------------------------------------------------------------------------------
    Tmp117 struct

    Tmp117<Twi0, 0x48, PINS::PC2> tmp117;
    Twi0 or Twialt, slave address (unshifted), optional power pin
------------------------------------------------------------------------------*/
template<typename Twi_, U8 Addr_ = 0x48>
struct Tmp117 {

    //============
        private:
    //============

    static Twi_& twi_;
    static inline bool isInit_{ false };

    enum REGISTERS { TEMP, CONFIG, HIGHLIMIT, LOWLIMIT, EEUNLOCK, EEPROM1,
                    EEPROM2, TEMPOFFSET, EEPROM3, DEVICEID = 15 };
    enum CONFIG_OFFSETS { SOFTRESET = 1, ALERTSEL, ALERTPOL, TAMODE, AVERAGE,
                    CONVCYCLE = 7, CONVMODE = 10, EEBUSY = 12, DATAREADY, LOWALERT,  HIGHALERT };
    enum EEUNLOCK_OFFSETS { EEBUSYu = 14, EUN = 15 };


                template<typename T> //T = U16 or I16
SA  read        (const U8 r, T& v) {
                    if( not isInit_ ) init();
                    volatile U8 rbuf[2] = { 0, 0 }; //value
                    U8 tbuf[1] = { r }; //register
                    bool tf = false;
                    if( twi_.writeRead( tbuf, rbuf) ){
                        v = (rbuf[0]<<8) bitor rbuf[1];
                        tf = true;
                    }
DebugFuncHeader();
Debug("  read reg: %d %s", r, tf ? "" : "[failed]");
if( tf ) Debug(" [0x%04X]", v ); 
Debug("\n");
                    return tf;
                }

                template<typename T> //T = U16 or I16
SA  write       (const U8 r, const T& v) {
                    if( not isInit_ ) init();
                    U8 vH = v>>8;   //avoid narrowing conversion
                    U8 vL = v;      //  error in array init
                    U8 buf[3] = { r, vH, vL };
                    bool tf = twi_.write( buf );
// DebugFuncHeader();
// Debug("  write reg: %d [0x%04X] %s\n", r, v, tf ? "ok" : "failed");
                    return tf;
                }

SA  configR     (U16& v)        { return read( CONFIG, v ); }
SA  configW     (U16 v)         { return write( CONFIG, v ); }
                //bitmask to clear, new value bitmask
SA  configWbm   (U16 bm, U16 nvm){
                    U16 v;
                    if( not configR( v ) ) return false;    //R
                    v  = (v bitand compl bm) bitor nvm;     //M
                    return configW( v );                    //W
                }

    //============
        public:
    //============

SA  init        ()              { twi_.init( Addr_ ); //K100
                                  nrf_delay_ms( 2 ); //startup time is 2ms
                                  isInit_ = true;
                                }

SA  deinit      ()              { twi_.deinit(); isInit_ = false; }

                                //these most likely end up in loops, so make it so it breaks
                                //the loop if a read failure

                                //can read and busy flag set = true,
                                //cannot read or flag clear = false
SA  isEEbusy    () -> bool      { U16 v; return configR( v ) ? v bitand (1<<EEBUSY) : false ; }
SA  isDataReady () -> bool      { U16 s = 0; 
                                  if( not configR(s) ) return false;
                                  return (s bitand (1<<DATAREADY));
                                } 

SA  reset       ()              { configW( 1<<SOFTRESET ); }

SA  continuous  ()              { return configWbm( 3<<CONVMODE, 0<<CONVMODE ); }
SA  shutdown    ()              { return configWbm( 3<<CONVMODE, 1<<CONVMODE ); }
SA  oneShot     ()              { return configWbm( 3<<CONVMODE, 3<<CONVMODE ); }

SA  averageOff  ()              { return configWbm( 3<<AVERAGE, 0<<AVERAGE ); }
SA  average8    ()              { return configWbm( 3<<AVERAGE, 1<<AVERAGE ); }
SA  average32   ()              { return configWbm( 3<<AVERAGE, 2<<AVERAGE ); }
SA  average64   ()              { return configWbm( 3<<AVERAGE, 3<<AVERAGE ); }

SA  eeUnlock    ()              { return write( EEUNLOCK, 0<<EUN ); }
SA  eeLock      ()              { return write( EEUNLOCK, 1<<EUN ); }

SA  id          (U16& v)        { return read( DEVICEID, v ); }
SA  highLimit   (U16& v)        { return write( HIGHLIMIT, v ); }
SA  lowLimit    (U16& v)        { return write( LOWLIMIT, v ); }
SA  tempRaw     (I16& v)        { return read( TEMP, v ); }


    /*
    conversion from raw to C or F x10,x100,x1000

    0.0078125*1.8 (0.0140625) degree F per count (without 32 degree offset)
    x10 = 9/64 = .140625, x100 = 45/32 = 1.40625, x1000 = 225/16 = 14.0625

    0.0078125 degree C per count (1/128 degree)
    x10 = 5/64 = .078125, x100 = 25/32 = 0.78125, x1000 = 125/16 = 7.8125

    normal mul/div- x100C -> v*78125/100000, x100F -> v*140625/100000 + 3200
    */

SA  x10F    (I16 v) -> I16      { return ((v * 9L)>>6) + 320; }
SA  x100F   (I16 v) -> I16      { return ((v * 45L)>>5) + 3200; }
SA  x1000F  (I16 v) -> int32_t  { return ((v * 225L)>>4) + 32000; }
SA  x10C    (I16 v) -> I16      { return (v * 5L)>>6; }
SA  x100C   (I16 v) -> I16      { return (v * 25L)>>5; }
SA  x1000C  (I16 v) -> int32_t  { return (v * 125L)>>4; }

};



#undef U32 
#undef U16 
#undef I16 
#undef U8 
#undef SA 
#undef SI
#undef SCA 

