#pragma once

/*-----------------------------------------------------------------------------
    includes
-----------------------------------------------------------------------------*/
#include "nRFconfig.hpp"

#include <cstdio>
#include <cstring>

#include "SEGGER_RTT.h"


/*------------------------------------------------------------------------------
    RTT
        output to segger RTT via swd 
        uses block(s) of memory to communictate via debugger (JLink)

        $ JLinkExe -device nrf52 -if swd -speed 400
        $ telnet localhost 19021
------------------------------------------------------------------------------*/
template<int N>
struct DevRtt {

    unsigned write(const char *buf, unsigned len){
        return SEGGER_RTT_Write(N, buf, len);
    }

    unsigned write(const char *buf){
        return SEGGER_RTT_WriteString(N, buf);
    }

};

/*------------------------------------------------------------------------------
    markup - embed markup codes into print format string

    Debug( "{Fgreen}the count is {Fred}{Bwhite}%d{normal}", count);

    Fcolor  = forground color
    Bcolor = backfround color
    each markup is contained in a single set of braces {Fblue}{italic}

    the markup codes are decoded at runtime
------------------------------------------------------------------------------*/

//previous method- ok, but the single letter keyword is limiting
#if 0 
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
        if( *str != '{' ){ 
            str++;
            continue;
        }
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
#endif

//simple hash to produce 16bit value from string
//the 16bits should prevent collisions but should check
//by running the markupColors through some script/app to verify
static constexpr uint16_t MKhash(const char* str){
    unsigned hash = 0;
    while ( *str ){ hash = hash * 33 + *str; str++; }
    return hash;    
}

using markupCodeT = struct {
    const u16 hash;
    const char* str;
};

inline const markupCodeT markupCodes[]{
    { MKhash("black"),      "0;0;0m" },
    { MKhash("red"),        "255;0;0m" },
    { MKhash("green"),      "0;255;0m" },
    { MKhash("yellow"),     "255;255;0m" },
    { MKhash("blue"),       "0;0;255m" },
    { MKhash("magenta"),    "255;0;255m" },
    { MKhash("cyan"),       "0;135;215m" },
    { MKhash("white"),      "255;255;255m" },
    { MKhash("orange"),     "255;99;71m" },
    { MKhash("purple"),     "143;0;211m" },
    //control
    { MKhash("cls"),        "2J" },
    { MKhash("home"),       "1;1H" },
    { MKhash("reset"),      "2J\033[1;1H\033[0m" }, //cls+home+normal
    //attributes
    { MKhash("italic"),     "3m" },
    { MKhash("normal"),     "0m" },
    { MKhash("underline"),  "4m" },

};

template<typename Dev_>
int Markup(Dev_ dev, const char* str){
    auto startStr = str;
    auto count = 0;
    for( ; *str; ){
        if( *str != '{' ){ 
            str++;
            continue;
        }
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

        //get hash of markup word
        u16 hash = 0;
        bool fg = (*str == 'F');
        bool bg = (*str == 'B');
        if( fg or bg ) str++;

        while ( *str and *str != '}' ){ hash = hash * 33 + *str; str++; }
        startStr = str;
        if( not *str ) break; //incomplete markup
        str++; startStr++; //skip }
        if( not markupON ) continue;

        for( auto& c : markupCodes ){
            if( c.hash != hash ) continue;
            count += dev.write( "\033[", 2 );
            if( fg ) count += dev.write( "38;2;", 5 );
            if( bg ) count += dev.write( "48;2;", 5 );
            count += dev.write( c.str );
            break;
        }
    }
    //print any remaining chars
    auto n = str - startStr;
    if( n ) count += dev.write( startStr, n );    
    return count;
}

/*------------------------------------------------------------------------------
    Print to device that has a pair of write functions -
        write(const char* buf, unsigned len) //for specific length
        write(const char*) //0 terminated

        embedded markup code enabled/disabled from the inline var
            bool markupON in nRFconfig.hpp

        if disabled, the existing markup code is ignored
------------------------------------------------------------------------------*/
template<typename Dev, typename...Ts>
int Print(Dev dev, const char* fmt, Ts...ts){
    char buf[512];
    int n = snprintf( buf, 512, fmt, ts... );
    if( n == 0 ) return 0;
    if( n < 512 or not markupON ) return Markup( dev, buf ); 
    //markup is on and all buffer used, so turn off markup and print 
    //what we have without ansi codes so we are not left with an incomplete
    //ansi code
    markupON = false;
    n = Markup( dev, buf );
    markupON = true;
    return n;
}
