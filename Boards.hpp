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
                led1G.blinkN(1,25,25,200);
                led2R.blinkN(1,25,25,200);
                led2G.blinkN(1,25,25,200);
                led2B.blinkN(1,25,25,200);
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

#endif //NRF52840

#ifdef NRF52810
/*------------------------------------------------------------------------------
    Laird module w/nrf52810
------------------------------------------------------------------------------*/
struct Laird52810 {

    //these pins are not init until init() is run
    //(could init each here, but do not want possible resulting constructor guard)
    SI Gpio<P0_7>  ledRed; //board label 1
    SI Gpio<P0_8>  ledGreen; //board label 2
    SI Gpio<P0_27, LOWISON>  sw1; //SW1

    //alias for other led names already in use
    SCA &ledGreen2  { ledGreen };
    SCA &ledRed1    { ledRed };

            //someone is required to run init to setup pins
SA  init    () {
                ledRed.init( OUTPUT );
                ledGreen.init( OUTPUT );
                sw1.init( INPUT, PULLUP );
            }

            //signal board is alive
SA  alive   () {
                ledGreen.blinkN(1,25,25,200);
                ledRed.blinkN(1,25,25,200);
            }

            //show error code via led's, skip leading 0's
            //show 1-15 blinks for each nibble (0x01-0x0f)
            //a zero will be a short green blink
SA  error   (uint16_t hex) {
                bool lz = true;
                for( auto i = 12; i >= 0; i -= 4 ){
                    uint8_t v = (hex>>i) bitand 0xf;
                    if( v == 0 and lz == true ) continue; //skip leading 0's
                    lz = false;
                    if( v ) ledRed.blinkN( v, 500 );
                    else ledGreen.blinkN( 1, 50 ); //0 is 1 short blue blink
                    nrf_delay_ms( 500 );
                }
            }

            //show a caution blink
SA  caution (uint16_t ms = 1) {
                ledRed.blinkN( 5, ms );
            }

            //show an ok blink
SA  ok      (uint16_t ms = 1) {
                ledGreen.blinkN( 5, ms );
            }

};

#endif

#undef SA
#undef SCA
#undef SI



//choose board in nrfConfig.hpp
//someone will need to run board.init()


#ifdef NRF52840_DONGLE
inline Pca10059 board;
#endif

#ifdef LAIRD_52810
inline Laird52810 board;
#endif
