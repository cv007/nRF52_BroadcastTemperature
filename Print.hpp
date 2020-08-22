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

};

/*
Debug( "{G}green{W}" );
Debug( "{cls}{home}{0x00ff00}green{0xffffff}" );
const char* fmt = "{G}green{W}" -> 
    "\033[38;2;0;255;0m"
    "green"
    "\033[38;2;0;0;0m"



char buf[256];
auto n = snprintf( buf, 256, fmt, ts... );
auto startIdx = 0;
for(auto i = 0; i < n; ){
    if( buf[i++] != '{' ){ continue; }
    if( i > startIdx ){ dev.write( &buf[startIdx], i-startIdx ); }
    if( buf[i] == '{' ){ startIdx = ++i; continue; 
    
    i = checkFmt( &buf[i] );
    startIdx = i;
}


*/


using codeT = struct {
    const char key;
    const char* str;
    const uint8_t siz;
};

inline const codeT codes[]{
    { 'K', "\033[38;2;0;0;0m", 13},
    { 'R', "\033[38;2;255;0;0m", 15 },
    { 'G', "\033[38;2;0;255;0m", 15 },
    { 'Y', "\033[38;2;255;255;0m", 17 },
    { 'B', "\033[38;2;0;0;255m", 15 },
    { 'M', "\033[38;2;255;0;255m", 17 },
    { 'C', "\033[38;2;0;135;215m", 17 },
    { 'W', "\033[38;2;255;255;255m", 19 },
    { 'O', "\033[38;2;255;150;0m", 17 },
    { '!', "\033[2J", 4 }, //cls
    { 'H', "\033[1;1H", 6 }, //home
    { '/', "\033[3m", 4 }, //italic
    { '|', "\033[0m", 4 }, //normal
    { '_', "\033[4m", 4 }, //underline
    { 'k', "\033[48;2;0;0;0m", 13},
    { 'r', "\033[48;2;255;0;0m", 15 },
    { 'g', "\033[48;2;0;255;0m", 15 },
    { 'y', "\033[48;2;255;255;0m", 17 },
    { 'b', "\033[48;2;0;0;255m", 15 },
    { 'm', "\033[48;2;255;0;255m", 17 },
    { 'c', "\033[48;2;0;135;215m", 17 },
    { 'w', "\033[48;2;255;255;255m", 19 },
    { 'o', "\033[48;2;255;150;0m", 17 }
};

template<typename Dev_>
int markup(Dev_ dev, const char* str){
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
        if( *str == '{' ){ count += dev.write( str, 1 ); str++; continue; }
        //print ansi sequence if enabled
        for( ; *str and *str != '}'; str++){
            for( auto& c : codes ){
                if( c.key != *str ) continue;
                if( markupON ) count += dev.write( c.str, c.siz );
                break;
            }
        }
        if( *str ) str++;
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
    //int n = 
    snprintf( buf, 256, fmt, ts... );
    return markup( dev, buf ); 
    //return dev.write( buf, n );
}

#if 0
/*------------------------------------------------------------------------------
    Ansi codes
------------------------------------------------------------------------------*/
#ifdef DEBUG_ANSI
#define CSI(s)       "\033[" STRINGIFY(s)
#define RGBfg(r,g,b) "\033[38;2;" STRINGIFY(r) ";" STRINGIFY(g) ";" STRINGIFY(b) "m"
#define RGBbg(r,g,b) "\033[48;2;" STRINGIFY(r) ";" STRINGIFY(g) ";" STRINGIFY(b) "m"
#else 
#define CSI(s)
#define RGBfg(r,g,b)
#define RGBbg(r,g,b)
#endif

#define BLACK       RGBfg(0,0,0)
#define RED         RGBfg(255,0,0)
#define GREEN       RGBfg(0,255,0)
#define YELLOW      RGBfg(255,255,0)
#define BLUE        RGBfg(0,0,255)
#define MAGENTA     RGBfg(255,0,255)
#define CYAN        RGBfg(0,135,215)
#define WHITE       RGBfg(255,255,255)
#define ORANGE      RGBfg(255,150,0)
//ansi backcolor
#define BLACKbg     RGBbg(0,0,0)
#define REDbg       RGBbg(255,0,0)
#define GREENbg     RGBbg(0,255,0)
#define YELLOWbg    RGBbg(255,255,0)
#define BLUEbg      RGBbg(0,0,255)
#define MAGENTAbg   RGBbg(255,0,255)
#define CYANbg      RGBbg(0,135,215)
#define WHITEbg     RGBbg(255,255,255)
#define ORANGEbg    RGBbg(255,150,0)
//reset colors/attributes/cls/home
#define RESET       CSI(0m) //"\033[0m"
#define CLS         CSI(2J) //"\033[2J"
#define HOME        CSI(1;1H) //"\033[1;1H"
#define ITALIC      CSI(3m) //"\033[3m";
#define NORMAL      CSI(0m) //"\033[0m";
#define UNDERLINE   CSI(4m) //"\033[4m";

#endif
