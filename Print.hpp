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

    //specified length (binary data)
    unsigned write(const char *buf, unsigned len){
        return SEGGER_RTT_Write(N, buf, len);
    }

    //string, 0  terminated
    unsigned write(const char *buf){
        return SEGGER_RTT_WriteString(N, buf);
    }

};


/*------------------------------------------------------------------------------
    Print to device that has a write function-
        write(const char*) //0 terminated
------------------------------------------------------------------------------*/
template<typename Dev, typename...Ts>
int Print(Dev dev, const char* fmt, Ts...ts){
    char buf[256];
    int n = snprintf( buf, 256, fmt, ts... );
    if( n == 0 ) return 0;
    dev.write( buf );
    return n;
}



/*-------------------------------------------------------------
    ansi codes

    colors need FG or BG preceeding
    << CLS << FG BLUE << BG WHITE << "fg blue, bg white"
    << ITALIC << FG RGB(200,100,50) << "italic rgb(200,100,50)"
--------------------------------------------------------------*/
#define ANSI_CSI            "\033["

#define FG                  ANSI_CSI "38;2;"
#define BG                  ANSI_CSI "48;2;"
#define RGB(r,g,b)          #r";"#g";"#b"m"


#define ANSI_CLS             ANSI_CSI "2J"
#define ANSI_HOME            ANSI_CSI "1;1H"
#define ANSI_RESET           ANSI_CLS ANSI_HOME ANSI_NORMAL
#define ANSI_ITALIC          ANSI_CSI "3m"
#define ANSI_NORMAL          ANSI_CSI "0m"
#define ANSI_UNDERLINE       ANSI_CSI "4m"

//SVG colors
#define ALICE_BLUE               RGB(240,248,255)
#define ANTIQUE_WHITE            RGB(250,235,215)
#define AQUA                     RGB(0,255,255)
#define AQUAMARINE               RGB(127,255,212)
#define AZURE                    RGB(240,255,255)
#define BEIGE                    RGB(245,245,220)
#define BISQUE                   RGB(255,228,196)
#define BLACK                    RGB(0,0,0)
#define BLANCHED_ALMOND          RGB(255,235,205)
#define BLUE                     RGB(0,0,255)
#define BLUE_VIOLET              RGB(138,43,226)
#define BROWN                    RGB(165,42,42)
#define BURLY_WOOD               RGB(222,184,135)
#define CADET_BLUE               RGB(95,158,160)
#define CHARTREUSE               RGB(127,255,0)
#define CHOCOLATE                RGB(210,105,30)
#define CORAL                    RGB(255,127,80)
#define CORNFLOWER_BLUE          RGB(100,149,237)
#define CORNSILK                 RGB(255,248,220)
#define CRIMSON                  RGB(220,20,60)
#define CYAN                     RGB(0,255,255)
#define DARK_BLUE                RGB(0,0,139)
#define DARK_CYAN                RGB(0,139,139)
#define DARK_GOLDEN_ROD          RGB(184,134,11)
#define DARK_GRAY                RGB(169,169,169)
#define DARK_GREEN               RGB(0,100,0)
#define DARK_KHAKI               RGB(189,183,107)
#define DARK_MAGENTA             RGB(139,0,139)
#define DARK_OLIVE_GREEN         RGB(85,107,47)
#define DARK_ORANGE              RGB(255,140,0)
#define DARK_ORCHID              RGB(153,50,204)
#define DARK_RED                 RGB(139,0,0)
#define DARK_SALMON              RGB(233,150,122)
#define DARK_SEA_GREEN           RGB(143,188,143)
#define DARK_SLATE_BLUE          RGB(72,61,139)
#define DARK_SLATE_GRAY          RGB(47,79,79)
#define DARK_TURQUOISE           RGB(0,206,209)
#define DARK_VIOLET              RGB(148,0,211)
#define DEEP_PINK                RGB(255,20,147)
#define DEEP_SKY_BLUE            RGB(0,191,255)
#define DIM_GRAY                 RGB(105,105,105)
#define DODGER_BLUE              RGB(30,144,255)
#define FIRE_BRICK               RGB(178,34,34)
#define FLORAL_WHITE             RGB(255,250,240)
#define FOREST_GREEN             RGB(34,139,34)
#define FUCHSIA                  RGB(255,0,255)
#define GAINSBORO                RGB(220,220,220)
#define GHOST_WHITE              RGB(248,248,255)
#define GOLD                     RGB(255,215,0)
#define GOLDEN_ROD               RGB(218,165,32)
#define GRAY                     RGB(128,128,128)
#define GREEN                    RGB(0,128,0)
#define GREEN_YELLOW             RGB(173,255,47)
#define HONEY_DEW                RGB(240,255,240)
#define HOT_PINK                 RGB(255,105,180)
#define INDIAN_RED               RGB(205,92,92)
#define INDIGO                   RGB(75,0,130)
#define IVORY                    RGB(255,255,240)
#define KHAKI                    RGB(240,230,140)
#define LAVENDER                 RGB(230,230,250)
#define LAVENDER_BLUSH           RGB(255,240,245)
#define LAWN_GREEN               RGB(124,252,0)
#define LEMON_CHIFFON            RGB(255,250,205)
#define LIGHT_BLUE               RGB(173,216,230)
#define LIGHT_CORAL              RGB(240,128,128)
#define LIGHT_CYAN               RGB(224,255,255)
#define LIGHT_GOLDENROD_YELLOW   RGB(250,250,210)
#define LIGHT_GRAY               RGB(211,211,211)
#define LIGHT_GREEN              RGB(144,238,144)
#define LIGHT_PINK               RGB(255,182,193)
#define LIGHT_SALMON             RGB(255,160,122)
#define LIGHT_SEA_GREEN          RGB(32,178,170)
#define LIGHT_SKY_BLUE           RGB(135,206,250)
#define LIGHT_SLATE_GRAY         RGB(119,136,153)
#define LIGHT_STEEL_BLUE         RGB(176,196,222)
#define LIGHT_YELLOW             RGB(255,255,224)
#define LIME                     RGB(0,255,0)
#define LIME_GREEN               RGB(50,205,50)
#define LINEN                    RGB(250,240,230)
#define MAGENTA                  RGB(255,0,255)
#define MAROON                   RGB(128,0,0)
#define MEDIUM_AQUAMARINE        RGB(102,205,170)
#define MEDIUM_BLUE              RGB(0,0,205)
#define MEDIUM_ORCHID            RGB(186,85,211)
#define MEDIUM_PURPLE            RGB(147,112,219)
#define MEDIUM_SEA_GREEN         RGB(60,179,113)
#define MEDIUM_SLATE_BLUE        RGB(123,104,238)
#define MEDIUM_SPRING_GREEN      RGB(0,250,154)
#define MEDIUM_TURQUOISE         RGB(72,209,204)
#define MEDIUM_VIOLET_RED        RGB(199,21,133)
#define MIDNIGHT_BLUE            RGB(25,25,112)
#define MINT_CREAM               RGB(245,255,250)
#define MISTY_ROSE               RGB(255,228,225)
#define MOCCASIN                 RGB(255,228,181)
#define NAVAJO_WHITE             RGB(255,222,173)
#define NAVY                     RGB(0,0,128)
#define OLD_LACE                 RGB(253,245,230)
#define OLIVE                    RGB(128,128,0)
#define OLIVE_DRAB               RGB(107,142,35)
#define ORANGE                   RGB(255,165,0)
#define ORANGE_RED               RGB(255,69,0)
#define ORCHID                   RGB(218,112,214)
#define PALE_GOLDENROD           RGB(238,232,170)
#define PALE_GREEN               RGB(152,251,152)
#define PALE_TURQUOISE           RGB(175,238,238)
#define PALE_VIOLET_RED          RGB(219,112,147)
#define PAPAYA_WHIP              RGB(255,239,213)
#define PEACH_PUFF               RGB(255,218,185)
#define PERU                     RGB(205,133,63)
#define PINK                     RGB(255,192,203)
#define PLUM                     RGB(221,160,221)
#define POWDER_BLUE              RGB(176,224,230)
#define PURPLE                   RGB(128,0,128)
#define REBECCA_PURPLE           RGB(102,51,153)
#define RED                      RGB(255,0,0)
#define ROSY_BROWN               RGB(188,143,143)
#define ROYAL_BLUE               RGB(65,105,225)
#define SADDLE_BROWN             RGB(139,69,19)
#define SALMON                   RGB(250,128,114)
#define SANDY_BROWN              RGB(244,164,96)
#define SEA_GREEN                RGB(46,139,87)
#define SEA_SHELL                RGB(255,245,238)
#define SIENNA                   RGB(160,82,45)
#define SILVER                   RGB(192,192,192)
#define SKY_BLUE                 RGB(135,206,235)
#define SLATE_BLUE               RGB(106,90,205)
#define SLATE_GRAY               RGB(112,128,144)
#define SNOW                     RGB(255,250,250)
#define SPRING_GREEN             RGB(0,255,127)
#define STEEL_BLUE               RGB(70,130,180)
#define TAN                      RGB(210,180,140)
#define TEAL                     RGB(0,128,128)
#define THISTLE                  RGB(216,191,216)
#define TOMATO                   RGB(255,99,71)
#define TURQUOISE                RGB(64,224,208)
#define VIOLET                   RGB(238,130,238)
#define WHEAT                    RGB(245,222,179)
#define WHITE                    RGB(255,255,255)
#define WHITE_SMOKE              RGB(245,245,245)
#define YELLOW                   RGB(255,255,0)
#define YELLOW_GREEN             RGB(154,205,50)

