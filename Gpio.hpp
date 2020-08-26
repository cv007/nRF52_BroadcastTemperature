#pragma once

#include <cstdint>
#include <cstdbool>

#include "nrf_delay.h"
#include "nRFconfig.hpp" //which mcu in use, for PIN enums

#define U32 uint32_t
#define SA [[gnu::always_inline]] static auto
// #define SA [[gnu::noinline]] static auto
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
    Gpio struct - template parameter of GPIO::PIN, 
                  and INVERT with a default of HIGHISON
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
    union  {    U32 OUT32; //0x50420
    struct {        U32         : PinN_;
                    U32 OUT     : 1;
                    U32         : 31-PinN_;
    };};
                U32 OUTSET;     //0x508
                U32 OUTCLR;     //0x50C
    union  {    U32 IN32;       //0x510
    struct {        U32         : PinN_;
                    U32 IN      : 1;
                    U32         : 31-PinN_;
    };};
    union  {    U32 DIR32;      //0x514
    struct {        U32         : PinN_;
                    U32 DIR     : 1;
                    U32         : 31-PinN_;
    };};
                U32 DIRSET;     //0x518
                U32 DIRCLR;     //0x51C
    union  {    U32 LATCH32;    //0x520
    struct {        U32         : PinN_;
                    U32 LATCH   : 1;
                    U32         : 31-PinN_;
    };};
    union  {    U32 DETECTMODE32; //0x524
    struct {        U32             : PinN_;
                    U32 DETECTMODE  : 1;
                    U32             : 31-PinN_;
    };};
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
SA  high        ()          { reg.OUTSET = bm_; } //register wide write
SA  low         ()          { reg.OUTCLR = bm_; } //register wide write
SA  off         ()          { if ( inv_ ) high(); else low(); }
SA  on          ()          { if ( inv_ ) low(); else high(); }
SA  on          (bool tf)   { if( tf ) on(); else off(); }
SA  toggle      ()          { if( reg.OUT ) low(); else high(); }

//------------
//  input
//------------
SA  isHigh      ()          { return reg.IN; }
SA  isLow       ()          { return not isHigh(); }
SA  isOn        ()          { return inv_ ? isLow() : isHigh(); }
SA  isOff       ()          { return not isOn(); }

//------------
//  properties
//------------
SA  pull        (PULL e)    { reg.PULL = e; }
SA  drive       (DRIVE e)   { reg.DRIVE = e; }
SA  sense       (SENSE e)   { reg.SENSE = e; }
SA  inputOff    ()          { reg.INBUF = 1; }
SA  inputOn     ()          { reg.INBUF = 0; }
SA  latchOff    ()          { reg.DETECTMODE = 0; }
SA  latchOn     ()          { reg.DETECTMODE = 1; }

//------------
//  status
//------------
SA  isOutput    ()          { return reg.DIRP; }
SA  isInbuf     ()          { return reg.INBUF == 0; } //1=off, 0=on
SA  isInput     ()          { return not isOutput() and isInbuf(); }
SA  isOutputOnly()          { return isOutput() and not isInbuf(); }
SA  isAnalog    ()          { return not isOutput() and not isInbuf(); }
SA  isLatch     ()          { return reg.lATCH; }
SA  clearLatch  ()          { reg.LATCH = 1; }

//------------
//  misc
//------------
                //so can get pin number from an existing instance
                //(if user only has the instance name, but needs the pin number)
                //the GPIO::PIN numbers in use match the pin number scheme used
                //in places that sets a peripheral pin
                //  PSEL.SCA = board.sca.pinNumber();
SCA pinNumber   ()          { return Pin_; }

SA  blinkN      (uint16_t n, uint32_t mson, uint32_t msoff = 0, uint32_t lastdelayms = 0) {
                    if( not isOutput() ) return;
                    if( not msoff ) msoff = mson;
                    while( n-- ){ 
                        toggle(); nrf_delay_ms( mson );  
                        toggle(); nrf_delay_ms( msoff );  
                    }
                    if( lastdelayms ) nrf_delay_ms( lastdelayms );
                    //will be in same state as when we arrived
                }

                //just a short blip
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
