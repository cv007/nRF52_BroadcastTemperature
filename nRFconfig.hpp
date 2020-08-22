#pragma once

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
    #define TEMPERATURE_TMP117
    //#define TEMPERATURE_SI7051
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
#define DEBUG_DEVICE            DevRtt<0>{} //change as needed, or comment out
inline bool markupON{true}; 

#ifdef DEBUG_DEVICE
#define Debug(...)              Print( DEBUG_DEVICE, __VA_ARGS__ )
#define DebugFuncHeader()       Debug("{G}[ %s:%d ::%s ]{W}\n", \
                                      __FILE__, __LINE__, __func__)
#else
#define Debug(...)
#define DebugFuncHeader()
#endif



