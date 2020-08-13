#pragma once

//== specify board in use, choose 1 ===

#define NRF52810_BL651_TEMP
// #define NRF52840_DONGLE






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



