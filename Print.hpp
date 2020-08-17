#pragma once

/*-----------------------------------------------------------------------------
    includes
-----------------------------------------------------------------------------*/
#include <cstdbool>
#include <cstdint>

#include "SEGGER_RTT.h"
#include <cstdio>

/*------------------------------------------------------------------------------
    Print to RTT
------------------------------------------------------------------------------*/
template<typename...Ts>
int Print(const char* fmt, Ts...ts){
    char buf[128];
    int ret = snprintf( buf, 128, fmt, ts... );
    SEGGER_RTT_WriteString(0, buf);
    return ret;
}