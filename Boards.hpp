#pragma once

#include <cstdbool>
#include <cstdint>

#include "nrf_delay.h"

#include "Gpio.hpp"

#define SA static auto
#define SCA static constexpr auto
#define SI static inline

#ifdef  NRF52840 //for 52840 based boards
/*------------------------------------------------------------------------------
    Pca10059 (nrf52 dongle, nrf52840)
------------------------------------------------------------------------------*/
struct Pca10059 {

    //these pins are not init until init() is run
    //(could init each here, but do not want possible resulting constructor guard)
    SI Gpio<P0_6, LOWISON>  led1G;
    SI Gpio<P1_9, LOWISON>  led2G;
    SI Gpio<P0_12, LOWISON> led2B;
    SI Gpio<P0_8, LOWISON>  led2R;
    SI Gpio<P1_6, LOWISON>  sw1;

    //'generic' led names
    SCA &ledGreen2  { led1G };
    SCA &ledRed1    { led2R };
    SCA &ledGreen1  { led2G };
    SCA &ledBlue1   { led2B };
    SCA &ledRed     { led2R };
    SCA &ledGreen   { led2G };
    SCA &ledBlue    { led2B };

            //someone is required to run init to setup pins
SA  init    () {
                led1G.init( OUTPUT );
                led2G.init( OUTPUT );
                led2R.init( OUTPUT );
                led2B.init( OUTPUT );
                sw1.init( INPUT, PULLUP );
            }

            //signal board is alive
SA  alive   () {
                led1G.blinkN(1,50,200); 
                led2R.blinkN(1,50,200);
                led2G.blinkN(1,50,200); 
                led2B.blinkN(1,50,200); 
            }

            //show error code via led's, skip leading 0's
            //show 1-15 blinks for each nibble (0x01-0x0f)
            //a zero will be a short blue blink
SA  error   (uint16_t hex) {
                bool lz = true;
                for( auto i = 12; i >= 0; i -= 4 ){
                    uint8_t v = (hex>>i) bitand 0xf;
                    if( v == 0 and lz == true ) continue; //skip leading 0's
                    lz = false;
                    if( v ) ledRed1.blinkN( v, 500 );
                    else ledBlue1.blinkN( 1, 50 ); //0 is 1 short blue blink
                    nrf_delay_ms( 500 );
                }
            }

            //show a caution blink
SA  caution (uint16_t ms = 1) {
                ledRed1.blinkN( 1, ms );
            }

            //show an ok blink
SA  ok      (uint16_t ms = 1) {
                ledGreen1.blinkN( 1, ms );
            }

};

#undef SA
#undef SCA
#undef SI

#endif //NRF52840


//choose board here for all who include this file
//someone will need to run board.init()
inline Pca10059 board;