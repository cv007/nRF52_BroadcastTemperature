#pragma once

//== specify board in use, choose 1 ===

#define LAIRD_52810
// #define NRF52840_DONGLE






#ifdef NRF52840_DONGLE
#include "nRF52840.hpp"
#define LAST_PAGE 0xDF000
#endif

#ifdef LAIRD_52810
#define LAST_PAGE 0x2F000
#include "nRF52810.hpp"
#endif



