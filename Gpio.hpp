#pragma once

#include <cstdint>
#include <cstdbool>

#include "nrf_delay.h"
#include "nRFconfig.hpp" //which mcu in use, for PIN enums

#define U32 uint32_t
#define SA [[gnu::always_inline]] static auto
//#define SA [[gnu::noinline]] static auto
#define SCA static constexpr auto

/*------------------------------------------------------------------------------
    GPIO namespace, for pin properties 
    (mcu pins are in mcu specific GPIO::PIN enum)
------------------------------------------------------------------------------*/
namespace GPIO {
    enum INVERT { HIGHISON, LOWISON };
    enum MODE   { INPUT, OUTPUT, ANALOG };
    enum PULL   { PULLOFF, PULLDOWN, PULLUP = 3 };
    enum DRIVE  { S0S1, H0S1, S0H1, H0H1, D0S1, D0H1, S0D1, H0D1 };
    enum SENSE  { SENSEOFF, SENSEHI = 2, SENSELO };
}
using namespace GPIO;

/*------------------------------------------------------------------------------
    Gpio struct - 1 template parameter of GPIO::PIN
------------------------------------------------------------------------------*/
template<PIN Pin_, INVERT Inv_ = HIGHISON>
struct Gpio {

//============
    private:
//============

    SCA     port_   { Pin_/32 };        //port 0 or 1
    SCA     base_   { 0x50000000+(0x300*port_) };
    SCA     pin_    { Pin_ bitand 31 }; //pin 0-31
    SCA     bm_     { 1<<pin_ };        //pin bitmask for multipin registers
    SCA     inv_    { Inv_ == LOWISON };//invert? (low=on?)

//------------
//  registers
//------------

    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wpedantic"
    template<uint8_t PinN_>
    struct Gpio_ {
                U32 unused1[0x504/4];
                U32 OUT;        //0x504
                U32 OUTSET;     //0x508
                U32 OUTCLR;     //0x50C
                U32 IN;         //0x510
                U32 DIR;        //0x514
                U32 DIRSET;     //0x518
                U32 DIRCLR;     //0x51C
                U32 LATCH;      //0x520
                U32 DETECTMODE; //0x524
                U32 unused2[(0x700-0x528)/4 + PinN_];
                //0x700+Pin_*4
    union  {    U32 PIN_CNF;
    struct {        U32 DIRP    : 1;
                    U32 INBUF   : 1; //1 = disable (default)
                    U32 PULL    : 2;
                    U32 unused3 : 4;
                    U32 DRIVE   : 3;
                    U32 unused4 : 5;
                    U32 SENSE   : 2;
    };};
    };
    #pragma GCC diagnostic pop

//------------
//  init
//------------

    /*
    for init() function or constructor, provide enum values in GPIO
    namespace to arguments in any order to set pin config
    will get default values if anything is not specified
    DETECT and LATCH are unchanged so will need to be set with other functions
    */
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wpedantic"
    using initT = struct {
        union  { U32 INIT_CNF;
        struct {    U32 DIRP    : 1;
                    U32 INBUF   : 1; //1 = disable (default)
                    U32 PULL    : 2;
                    U32 unused3 : 4;
                    U32 DRIVE   : 3;
                    U32 unused4 : 5;
                    U32 SENSE   : 2;
        };};
    };
    #pragma GCC diagnostic pop

                template<typename ...Ts>
SCA init_       (initT& it, MODE e, Ts... ts) {
                    it.DIRP = (e == OUTPUT);
                    //let output use in buffer so if use the is* functions
                    //intended for input, they will still work for output
                    it.INBUF = (e == ANALOG); //inbif off only for analog
                    init_(it, ts...); 
                }

                template<typename ...Ts>
SCA init_       (initT& it, PULL e, Ts... ts) {
                    it.PULL = e;
                    init_(it, ts...); 
                }

                template<typename ...Ts>
SCA init_       (initT& it, DRIVE e, Ts... ts) {
                    it.DRIVE = e;
                    init_(it, ts...); 
                }

                template<typename ...Ts>
SCA init_       (initT& it, SENSE e, Ts... ts) {
                    it.SENSE = e;
                    init_(it, ts...); 
                }

SCA init_       (initT& it) { //no more arguments, now set PIN_CNF from accumulated values
                    if( it.DIRP ) off(); //if output, set to off
                    reg.PIN_CNF = it.INIT_CNF;
                }

//============
    public:
//============

    //give public access to registers
    static inline volatile Gpio_<pin_>&
    reg { *(reinterpret_cast<Gpio_<pin_>*>(base_)) };

    Gpio        ()          {} //init later

                template<typename ...Ts>
    Gpio        (Ts... ts)  { init( ts... ); } //init now

//------------
//  init
//------------
                template<typename ...Ts>
SA  init        (Ts... ts)  { initT it{2}; init_(it, ts...); }

//------------
//  output
//------------
SA  high        ()          { reg.OUTSET = bm_; }
SA  low         ()          { reg.OUTCLR = bm_; }
SA  off         ()          { if constexpr( inv_ ) high(); else low(); }
SA  on          ()          { if constexpr( inv_ ) low(); else high(); }
SA  on          (bool tf)   { if( tf ) on(); else off(); }
SA  toggle      ()          { if( reg.OUT bitand bm_ ) low(); else high(); }

//------------
//  input
//------------
SA  isHigh      ()          { return reg.IN bitand bm_; }
SA  isLow       ()          { return not isHigh(); }
SA  isOn        ()          { if constexpr( inv_ ) return isLow(); else return isHigh(); }
SA  isOff       ()          { return not isOn(); }

//------------
//  properties
//------------
SA  pull        (PULL e)    { reg.PULL = e; }
SA  drive       (DRIVE e)   { reg.DRIVE = e; }
SA  sense       (SENSE e)   { reg.SENSE = e; }
SA  inputOff    ()          { reg.INBUF = 1; }
SA  inputOn     ()          { reg.INBUF = 0; }
SA  latchOff    ()          { reg.DETECTMODE and_eq compl bm_; }
SA  latchOn     ()          { reg.DETECTMODE or_eq bm_; }

//------------
//  status
//------------
SA  isOutput    ()          { return reg.DIRP; }
SA  isInput     ()          { return (reg.PIN_CNF bitand 3) == 0; }
SA  isAnalog    ()          { return (reg.PIN_CNF bitand 3) == 2; }
SA  isLatch     ()          { return reg.lATCH bitand bm_; }
SA  clearLatch  ()          { reg.LATCH = bm_; }

//------------
//  misc
//------------
SA  blinkN      (uint16_t n, uint32_t periodms) {
                    if( not isOutput() ) return;
                    uint32_t mshalf = periodms>>1;
                    if( not mshalf ) mshalf = 1;
                    uint32_t n2 = n*2; //will also keep return led state same as before
                    while( n2-- ){ toggle(); nrf_delay_ms( mshalf ); }
                    //will be in same state as when we arrived
                }

SA  blink1      () { blinkN(1,1); }

                //for switches, wait for sw release of N ms, or a max of 3 sec if no release
SA  debounce    (uint16_t ms = 50) {
                    if( not isInput() ) return;
                    uint16_t offcount = ms;
                    uint16_t allcount = 3000; //limit, can't wait here all day
                    while( nrf_delay_ms( 1 ), offcount-- and allcount-- ){
                        if( isOn() ) offcount = ms;
                    }
                }

};
/*----------------------------------------------------------------------------*/

#undef U32 
#undef SA 
#undef SCA 
