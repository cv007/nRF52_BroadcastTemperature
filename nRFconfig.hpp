#pragma once

//common includes
#include <cstdbool>
#include <cstdint>

//common function types
#define SA static auto
#define SCA static constexpr auto
#define SI static inline

//our own types, to use include this file
using u8  = uint8_t;
using i8  =  int8_t;
using u16 = uint16_t;
using i16 =  int16_t;
using u32 = uint32_t;
using i32 =  int32_t;
using u64 = uint64_t;
using i64 =  int64_t;

SCA operator "" _0b  (u64 v) { return (bool)v; }
SCA operator "" _u8  (u64 v) { return ( u8)v; }
SCA operator "" _i8  (u64 v) { return ( i8)v; }
SCA operator "" _u16 (u64 v) { return (u16)v; }
SCA operator "" _i16 (u64 v) { return (i16)v; }
SCA operator "" _u32 (u64 v) { return (u32)v; }
SCA operator "" _i32 (u64 v) { return (i32)v; }
SCA operator "" _u64 (u64 v) { return (u64)v; }
SCA operator "" _i64 (u64 v) { return (i64)v; }




#include "app_timer.h" //rtc count for Debug

/*------------------------------------------------------------------------------
    specify board in use, choose 1
------------------------------------------------------------------------------*/
#define NRF52810_BL651_TEMP
// #define NRF52840_DONGLE


/*------------------------------------------------------------------------------
    using above setting
------------------------------------------------------------------------------*/
#ifdef NRF52840_DONGLE
    #include "nRF52840.hpp"
    #define LAST_PAGE 0xDF000
    #define TEMPERATURE_INTERNAL
#endif

#ifdef NRF52810_BL651_TEMP
    #define LAST_PAGE 0x2F000
    // #define TEMPERATURE_TMP117
    #define TEMPERATURE_SI7051
    #include "nRF52810.hpp"
#endif


/*------------------------------------------------------------------------------
    set debug device, create Debug alias to Print

        Debug("[%s:%d] %s\n", __FILE__, __LINE__, __func__);
        DebugFuncHeader(); //same as above
        Debug("\tmyVar: %d", myVar);

        debug device in Print.hpp
        comment out DEBUG_DEVICE to turn off debug output
        set markupON to false if markup code not wanted (markup code bypassed)
------------------------------------------------------------------------------*/
#ifdef NRF52810_BL651_TEMP
#define DEBUG_DEVICE            DevRtt<0>{} //change as needed, or comment out
#endif
inline bool markupON{true}; 

#ifdef DEBUG_DEVICE
#define Debug(...)              Print( DEBUG_DEVICE, __VA_ARGS__ )
//using app timer rtc1 as system time, is /2 so 16384 per sec
#define DebugFuncHeader()       do { u32 t = app_timer_cnt_get(); \
                                Debug("{Fgreen}[%04d.%06d][%s:%d ::%s]{Fwhite}\n", \
                                t/16384, (t%16384) * 61, \
                                __FILE__, __LINE__, __func__); } while(0)
#else
#define Debug(...)
#define DebugFuncHeader()
#endif



