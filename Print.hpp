#pragma once

/*-----------------------------------------------------------------------------
    includes
-----------------------------------------------------------------------------*/
#include <cstdbool>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "SEGGER_RTT.h"
#include "nRFconfig.hpp"

/*------------------------------------------------------------------------------
    RTT
        output to segger RTT via swd 
        uses block(s) of memory to communictate via debugger (JLink)

        $ JLinkExe -device nrf52 -if swd -speed 400
        $ telnet localhost 19021
------------------------------------------------------------------------------*/
template<int N>
struct DevRtt {

    unsigned write(const void *buf, unsigned len){
        return SEGGER_RTT_Write(N, buf, len);
    }

    unsigned write(const char *buf){
        return SEGGER_RTT_WriteString(N, buf);
    }

};

/*------------------------------------------------------------------------------
    markup - embed markup codes into print format string

    Debug( "{G}the count is {Rw/}%d{|Wk}", count);
        G = green forground, R = red foreground, w = white background, 
        / = italics, | = normal, W = white forground, k = black background

    the markup codes are decoded at runtime
------------------------------------------------------------------------------*/
using markupCodeT = struct {
    const char key;     //embedded char
    const char* str;    //ansi sequence
};

inline const markupCodeT markupCodes[]{
    //foreground colors
    { 'K', "\033[38;2;0;0;0m" },
    { 'R', "\033[38;2;255;0;0m" },
    { 'G', "\033[38;2;0;255;0m" },
    { 'Y', "\033[38;2;255;255;0m" },
    { 'B', "\033[38;2;0;0;255m" },
    { 'M', "\033[38;2;255;0;255m" },
    { 'C', "\033[38;2;0;135;215m" },
    { 'W', "\033[38;2;255;255;255m" },
    { 'O', "\033[38;2;255;150;0m" },
    //control
    { '*', "\033[2J" }, //cls
    { '^', "\033[1;1H" }, //home
    { '!', "\033[2J\033[1;1H\033[0m" }, //cls+home+normal
    //attributes
    { '/', "\033[3m" }, //italic
    { '|', "\033[0m" }, //normal
    { '_', "\033[4m" }, //underline
    //background colors
    { 'k', "\033[48;2;0;0;0m" },
    { 'r', "\033[48;2;255;0;0m" },
    { 'g', "\033[48;2;0;255;0m" },
    { 'y', "\033[48;2;255;255;0m" },
    { 'b', "\033[48;2;0;0;255m" },
    { 'm', "\033[48;2;255;0;255m" },
    { 'c', "\033[48;2;0;135;215m" },
    { 'w', "\033[48;2;255;255;255m" },
    { 'o', "\033[48;2;255;150;0m" }
};

template<typename Dev_>
int Markup(Dev_ dev, const char* str){
    auto startStr = str;
    auto count = 0;
    for( ; *str; ){
        if( *str != '{' ){ str++; continue; }
        //found {
        //print previous chars up to {
        auto n = str - startStr;
        if( n ) count += dev.write( startStr, n );
        str++; //skip {
        //check for escaped {
        if( *str == '{' ){ 
            count += dev.write( str, 1 ); 
            str++; 
            continue; 
        }
        //print ansi sequence if enabled, else just find } or end
        for( ; *str and *str != '}'; str++){
            if( not markupON ) continue;
            for( auto& c : markupCodes ){
                if( c.key != *str ) continue;
                count += dev.write( c.str );
                break;
            }
        }
        if( *str ) str++; //skip }
        startStr = str;
    }
    //print any remaining chars
    auto n = str - startStr;
    if( n ) count += dev.write( startStr, n );    
    return count;
}


/*------------------------------------------------------------------------------
    Print to device with a write function -
        write(const void* buf, unsigned len)
------------------------------------------------------------------------------*/
template<typename Dev, typename...Ts>
int Print(Dev dev, const char* fmt, Ts...ts){
    char buf[256];
    int n = snprintf( buf, 256, fmt, ts... );
    if( n < 256 or not markupON ) return Markup( dev, buf ); 
    //markup is on and all buffer used, so turn off markup and print what we have
    markupON = false;
    n = Markup( dev, buf );
    markupON = true;
    return n;
}
