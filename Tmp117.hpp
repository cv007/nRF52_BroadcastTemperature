#pragma once

#include "nRFconfig.hpp"

#include "nrf_delay.h"

#include "Twim.hpp"
#include "Print.hpp"

/*------------------------------------------------------------------------------
    Tmp117 struct

    Tmp117<Twi, [address]> tmp117;

    TMP117 - Texas Instruments temperature IC
------------------------------------------------------------------------------*/
template<typename Twi_, u8 Addr_ = 0x48>
struct Tmp117 {

    static_assert(Addr_ >= 0x48 and Addr_ <= 0x4B, "invalid Tmp117 address");

    //============
        private:
    //============

    static Twi_& twi_;
    static inline bool isInit_{ false };

            //registers
    enum    { TEMP, CONFIG, HIGHLIMIT, LOWLIMIT, EEUNLOCK, EEPROM1,
              EEPROM2, TEMPOFFSET, EEPROM3, DEVICEID = 15 };
            //bit positions in CONFIG
    enum    { SOFTRESET = 1, ALERTSEL, ALERTPOL, TAMODE, AVERAGE,
              CONVCYCLE = 7, CONVMODE = 10, EEBUSY = 12, DATAREADY, LOWALERT,  HIGHALERT };
            //eeunlock bits
    enum    { EEBUSYu = 14, EUN = 15 };


                template<typename T> //T = u16 or i16
SA  read        (const u8 r, T& v) {
                    if( not isInit_ ) init();
                    u8 rbuf[2]; //if want to init this for some reason, make it volatile
                    u8 tbuf[1] = { r }; //register
                    bool tf = false;
                    if( twi_.writeRead( tbuf, rbuf) ){
                        v = (rbuf[0]<<8) bitor rbuf[1];
                        tf = true;
                    }
                    // DebugFuncHeader();
                    // DebugRtt << "  read reg: " r << " " << (tf ? " " : "[failed]";
                    // if( tf ) DebugRtt << '[' << Hex0 << setwf(4,'0') << v << ']';
                    // DebugRtt << endlc;
                    return tf;
                }

                template<typename T> //T = u16 or i16
SA  write       (const u8 r, const T& v) {
                    if( not isInit_ ) init();
                    u8 vH = v>>8;   //avoid narrowing conversion
                    u8 vL = v;      //  error in array init
                    u8 buf[3] = { r, vH, vL };
                    bool tf = twi_.write( buf );
                    // DebugFuncHeader();
                    // DebugRtt << "  write reg: " r << " [" << Hex0 << setwf(4,'0') << v << ']' << (tf ? " ok" : " failed" << clear;
                    return tf;
                }

SA  configR     (u16& v)        { return read( CONFIG, v ); }
SA  configW     (u16 v)         { return write( CONFIG, v ); }
                //bitmask to clear, new value bitmask
SA  configWbm   (u16 bm, u16 nvm){
                    u16 v;
                    if( not configR( v ) ) return false;    //R
                    v  = (v bitand compl bm) bitor nvm;     //M
                    return configW( v );                    //W
                }

    //============
        public:
    //============

SA  init        ()              { twi_.init( Addr_, twi_.K400 );
                                  nrf_delay_ms( 2 ); //startup time is 2ms
                                  isInit_ = true;
                                }

SA  deinit      ()              { twi_.deinit(); isInit_ = false; }

                                //these most likely end up in loops, so make it so it breaks
                                //the loop if a read failure

                                //can read and busy flag set = true,
                                //cannot read or flag clear = false
SA  isEEbusy    () -> bool      { u16 v; return configR( v ) ? v bitand (1<<EEBUSY) : false ; }
SA  isDataReady () -> bool      { u16 s = 0; 
                                  if( not configR(s) ) return false;
                                  return (s bitand (1<<DATAREADY));
                                } 

SA  reset       ()              { configW( 1<<SOFTRESET ); }

SA  continuous  ()              { return configWbm( 3<<CONVMODE, 0<<CONVMODE ); }
SA  shutdown    ()              { return configWbm( 3<<CONVMODE, 1<<CONVMODE ); }
SA  oneShot     ()              { return configWbm( 3<<CONVMODE, 3<<CONVMODE ); }
SA  oneShot1    ()              { return configWbm( 3<<CONVMODE|3<<AVERAGE, 3<<CONVMODE|0<<AVERAGE ); }
SA  oneShot8    ()              { return configWbm( 3<<CONVMODE|3<<AVERAGE, 3<<CONVMODE|1<<AVERAGE ); }
SA  oneShot32   ()              { return configWbm( 3<<CONVMODE|3<<AVERAGE, 3<<CONVMODE|2<<AVERAGE ); }
SA  oneShot64   ()              { return configWbm( 3<<CONVMODE|3<<AVERAGE, 3<<CONVMODE|3<<AVERAGE ); }

SA  averageOff  ()              { return configWbm( 3<<AVERAGE, 0<<AVERAGE ); }
SA  average8    ()              { return configWbm( 3<<AVERAGE, 1<<AVERAGE ); }
SA  average32   ()              { return configWbm( 3<<AVERAGE, 2<<AVERAGE ); }
SA  average64   ()              { return configWbm( 3<<AVERAGE, 3<<AVERAGE ); }

SA  eeUnlock    ()              { return write( EEUNLOCK, 0<<EUN ); }
SA  eeLock      ()              { return write( EEUNLOCK, 1<<EUN ); }

SA  id          (u16& v)        { return read( DEVICEID, v ); }
SA  highLimit   (u16& v)        { return write( HIGHLIMIT, v ); }
SA  lowLimit    (u16& v)        { return write( LOWLIMIT, v ); }
SA  tempRaw     (i16& v)        { return read( TEMP, v ); }


    /*
    conversion from raw to C or F x10,x100,x1000

    0.0078125*1.8 (0.0140625) degree F per count (without 32 degree offset)
    x10 = 9/64 = .140625, x100 = 45/32 = 1.40625, x1000 = 225/16 = 14.0625

    0.0078125 degree C per count (1/128 degree)
    x10 = 5/64 = .078125, x100 = 25/32 = 0.78125, x1000 = 125/16 = 7.8125

    normal mul/div- x100C -> v*78125/100000, x100F -> v*140625/100000 + 3200
    */

SA  x10F    (i16 v) -> i16      { return ((v * 9L)>>6) + 320; }
SA  x100F   (i16 v) -> i16      { return ((v * 45L)>>5) + 3200; }
SA  x1000F  (i16 v) -> int32_t  { return ((v * 225L)>>4) + 32000; }
SA  x10C    (i16 v) -> i16      { return (v * 5L)>>6; }
SA  x100C   (i16 v) -> i16      { return (v * 25L)>>5; }
SA  x1000C  (i16 v) -> int32_t  { return (v * 125L)>>4; }

};


